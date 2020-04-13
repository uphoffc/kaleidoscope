#ifndef K_VISITOR_CODEGEN_H_
#define K_VISITOR_CODEGEN_H_

#include <unordered_map>
#include <stack>

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include <md/visit.hpp>
#include "AST.h"

using namespace llvm;

class CodeGen {
private:
  LLVMContext context;
  IRBuilder<> builder;
  std::unique_ptr<Module> module;
  std::unique_ptr<legacy::FunctionPassManager> fpm;

  std::unordered_map<std::string, AllocaInst*> namedValues;
  std::stack<Value*> values;
  std::stack<Function*> prototypes;

  Value* logError(char const* str) {
    std::cerr << "Error: " << str << std::endl;
    return nullptr;
  }

  std::vector<Function*> functions;

  Value* visitAndGet(ExprAST& expr) {
    md::visit(*this, expr);
    Value* v = values.top();
    values.pop();
    return v;
  }

  AllocaInst* CreateEntryBlockAlloca(Function* f, std::string const& varName) {
    IRBuilder<> tmp(&f->getEntryBlock(), f->getEntryBlock().begin());
    return tmp.CreateAlloca(Type::getDoubleTy(context), 0, varName);
  }

public:
  CodeGen()
    : builder(context),
      module(std::make_unique<Module>("my cool jit", context)),
      fpm(std::make_unique<legacy::FunctionPassManager>(module.get()))
  {
    fpm->add(createPromoteMemoryToRegisterPass());
    fpm->add(createInstructionCombiningPass());
    fpm->add(createReassociatePass());
    fpm->add(createGVNPass());
    fpm->add(createCFGSimplificationPass());
    fpm->doInitialization();
  }

  auto const& getFunctions() const {
    return functions;
  }

  void operator()(ExprAST& node) {}

  void operator()(NumberExprAST& node) {
    Value* v = ConstantFP::get(context, APFloat(node.getNumber()));
    values.push(v);
  }

  void operator()(VariableExprAST& node) {
    Value* v = namedValues[node.getName()];
    if (!v) {
      logError("Unknown variable name");
    }
    v = builder.CreateLoad(v, node.getName());
    values.push(v);
  }

  void operator()(BinaryExprAST& node) {
    if (node.getOp() == '=') {
      VariableExprAST* lhsE = dynamic_cast<VariableExprAST*>(&node.getLHS());
      if (!lhsE) {
        logError("destination of '=' must be a variable");
        values.push(nullptr);
        return;
      }
      Value* rhs = visitAndGet(node.getRHS());
      if (!rhs) {
        values.push(nullptr);
        return;
      }
      Value* variable = namedValues[lhsE->getName()];
      if (!variable) {
        logError("Unknown variable name");
        values.push(nullptr);
        return;
      }

      builder.CreateStore(rhs, variable);
      values.push(rhs);
      return;
    }

    md::visit(*this, node.getLHS());
    md::visit(*this, node.getRHS());
    Value* rhs = values.top();
    values.pop();
    Value* lhs = values.top();
    values.pop();

    Value* v = nullptr;

    if (lhs && rhs) {
      switch (node.getOp()) {
        case '+':
          v = builder.CreateFAdd(lhs, rhs, "addtmp");
          break;
        case '-':
          v = builder.CreateFSub(lhs, rhs, "subtmp");
          break;
        case '*':
          v = builder.CreateFMul(lhs, rhs, "multmp");
          break;
        case '<':
          v = builder.CreateFCmpULT(lhs, rhs, "cmptmp");
          v = builder.CreateUIToFP(v, Type::getDoubleTy(context), "booltmp");
          break;
        default:
          logError("Unknown operator ");
          break;
      }
    }
    values.push(v);
  }
  void operator()(CallExprAST& node) {
    Value* v = nullptr;
    Function* calleeF = module->getFunction(node.getCallee());
    if (!calleeF) {
      logError("Unknown function referenced");
    } else if (calleeF->arg_size() != node.getArgs().size()) {
      logError("Incorrect number of arguments passed");
    } else {
      std::vector<Value*> args;
      for (auto& arg : node.getArgs()) {
        md::visit(*this, *arg);
        Value* result = values.top();
        values.pop();
        if (result) {
          args.push_back(values.top());
        } else {
          break;
        }
      }
      if (args.size() == calleeF->arg_size()) {
        v = builder.CreateCall(calleeF, args, "calltmp");
      }
    }
    values.push(v);
  }
  void operator()(IfExprAST& node) {
    Value* Cond = visitAndGet(node.getCond());
    if (!Cond) {
      values.push(nullptr);
      return;
    }

    Cond = builder.CreateFCmpONE(Cond, ConstantFP::get(context, APFloat(0.0)), "ifcond");

    Function* f = builder.GetInsertBlock()->getParent();

    BasicBlock* ThenBB = BasicBlock::Create(context, "then", f);
    BasicBlock* ElseBB = BasicBlock::Create(context, "else");
    BasicBlock* MergeBB = BasicBlock::Create(context, "ifcont");

    builder.CreateCondBr(Cond, ThenBB, ElseBB);

    builder.SetInsertPoint(ThenBB);

    Value* Then = visitAndGet(node.getThen());
    if (!Then) {
      values.push(nullptr);
      return;
    }

    builder.CreateBr(MergeBB);
    ThenBB = builder.GetInsertBlock();

    f->getBasicBlockList().push_back(ElseBB);
    builder.SetInsertPoint(ElseBB);

    Value* Else = visitAndGet(node.getElse());
    if (!Else) {
      values.push(nullptr);
      return;
    }

    builder.CreateBr(MergeBB);
    ElseBB = builder.GetInsertBlock();

    f->getBasicBlockList().push_back(MergeBB);
    builder.SetInsertPoint(MergeBB);
    PHINode* phi = builder.CreatePHI(Type::getDoubleTy(context), 2, "iftmp");

    phi->addIncoming(Then, ThenBB);
    phi->addIncoming(Else, ElseBB);

    values.push(phi);
  }
  void operator()(ForExprAST& node) {
    Function* f = builder.GetInsertBlock()->getParent();

    AllocaInst* Alloca = CreateEntryBlockAlloca(f, node.getVarName());

    Value* Start = visitAndGet(node.getStart());
    if (!Start) {
      values.push(nullptr);
      return;
    }

    builder.CreateStore(Start, Alloca);

    BasicBlock* PreheaderBB = builder.GetInsertBlock();
    BasicBlock* LoopBB = BasicBlock::Create(context, "loop", f);

    builder.CreateBr(LoopBB);

    builder.SetInsertPoint(LoopBB);

    PHINode* Variable = builder.CreatePHI(Type::getDoubleTy(context), 2, node.getVarName());
    Variable->addIncoming(Start, PreheaderBB);

    AllocaInst* OldVal = namedValues[node.getVarName()];
    namedValues[node.getVarName()] = Alloca;

    Value* Body = visitAndGet(node.getBody());
    if (!Body) {
      values.push(nullptr);
      return;
    }

    Value* Step = nullptr;
    if (node.getStep()) {
      Step = visitAndGet(node.getStep()->get());
      if (!Step) {
        values.push(nullptr);
        return;
      }
    } else {
      Step = ConstantFP::get(context, APFloat(1.0));
    }

    Value* End = visitAndGet(node.getEnd());
    if (!End) {
      values.push(nullptr);
      return;
    }

    Value* CurVar = builder.CreateLoad(Alloca);
    Value* NextVar = builder.CreateFAdd(CurVar, Step, "nextvar");
    builder.CreateStore(NextVar, Alloca);

    End = builder.CreateFCmpONE(End, ConstantFP::get(context, APFloat(0.0)), "loopcond");

    BasicBlock* LoopEndBB = builder.GetInsertBlock();
    BasicBlock* AfterBB = BasicBlock::Create(context, "afterloop", f);

    builder.CreateCondBr(End, LoopBB, AfterBB);

    builder.SetInsertPoint(AfterBB);

    Variable->addIncoming(NextVar, LoopEndBB);

    if (OldVal) {
      namedValues[node.getVarName()] = OldVal;
    } else {
      namedValues.erase(node.getVarName());
    }

    values.push(Constant::getNullValue(Type::getDoubleTy(context)));
  }
  void operator()(VarExprAST& node) {
    std::vector<AllocaInst*> OldBindings;

    Function* f = builder.GetInsertBlock()->getParent();

    for (auto& entry : node.getVarNames()) {
      std::string const& VarName = entry.first;
      ExprAST* Init = entry.second.get();

      Value* InitVal;
      if (Init) {
        InitVal = visitAndGet(*Init);
        if (!InitVal) {
          values.push(nullptr);
          return;
        }
      } else {
        InitVal = ConstantFP::get(context, APFloat(0.0));
      }

      AllocaInst* Alloca = CreateEntryBlockAlloca(f, VarName);
      builder.CreateStore(InitVal, Alloca);
      OldBindings.push_back(namedValues[VarName]);
      namedValues[VarName] = Alloca;
    }

    Value* BodyVal = visitAndGet(node.getBody());
    if (!BodyVal) {
      values.push(nullptr);
      return;
    }

    auto it = OldBindings.begin();
    for (auto& entry : node.getVarNames()) {
      namedValues[entry.first] = *it++;
    }

    values.push(BodyVal);
  }
  void operator()(PrototypeAST& node) {
    auto const& args = node.getArgs();
    std::vector<Type*> doubles(args.size(), Type::getDoubleTy(context));
    FunctionType* ft = FunctionType::get(Type::getDoubleTy(context), doubles, false);
    Function* f = Function::Create(ft, Function::ExternalLinkage, node.getName(), module.get());

    assert(f->arg_size() == args.size());
    auto it = args.begin();
    for (auto& arg : f->args()) {
      arg.setName(*it++);
    }

    prototypes.push(f);
  }
  void operator()(FunctionAST& node) {
    Function* f = module->getFunction(node.getPrototype().getName());

    if (!f) {
      md::visit(*this, node.getPrototype());
      f = prototypes.top();
      prototypes.pop();
    }

    if (f && f->empty()) {
      BasicBlock* bb = BasicBlock::Create(context, "entry", f);
      builder.SetInsertPoint(bb);

      namedValues.clear();
      for (auto& arg : f->args()) {
        AllocaInst* Alloca = CreateEntryBlockAlloca(f, arg.getName());
        builder.CreateStore(&arg, Alloca);
        namedValues[arg.getName()] = Alloca;
      }

      md::visit(*this, node.getBody());
      Value* retVal = values.top();
      values.pop();
      if (retVal) {
        builder.CreateRet(retVal);
        llvm::verifyFunction(*f, &llvm::errs());
        fpm->run(*f);
        functions.push_back(f);
      } else {
        f->eraseFromParent();
      }
    } else {
      logError("Function cannot be redefined");
    }
  }
};

#endif
