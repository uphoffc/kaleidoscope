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

  Value* logError(char const* str) {
    std::cerr << "Error: " << str << std::endl;
    return nullptr;
  }

  Function* logErrorF(char const* str) {
    logError(str);
    return nullptr;
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

  Value* operator()(ExprAST& node) { return nullptr; }

  Value* operator()(NumberExprAST& node) {
    return ConstantFP::get(context, APFloat(node.getNumber()));
  }

  Value* operator()(VariableExprAST& node) {
    Value* v = namedValues[node.getName()];
    if (!v) {
      return logError("Unknown variable name");
    }
    return builder.CreateLoad(v, node.getName());
  }

  Value* operator()(BinaryExprAST& node) {
    if (node.getOp() == '=') {
      VariableExprAST* lhsE = dynamic_cast<VariableExprAST*>(&node.getLHS());
      if (!lhsE) {
        return logError("destination of '=' must be a variable");
        return nullptr;
      }
      Value* rhs = md::visit(*this, node.getRHS());
      if (!rhs) {
        return nullptr;
      }
      Value* variable = namedValues[lhsE->getName()];
      if (!variable) {
        return logError("Unknown variable name");
      }

      builder.CreateStore(rhs, variable);
      return rhs;
    }

    Value* lhs = md::visit(*this, node.getLHS());
    Value* rhs = md::visit(*this, node.getRHS());

    if (!lhs || !rhs) {
      return nullptr;
    }

    Value* v = nullptr;
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
        v = logError("Unknown operator");
        break;
    }
    return v;
  }
  Value* operator()(CallExprAST& node) {
    Function* calleeF = module->getFunction(node.getCallee());
    if (!calleeF) {
      return logError("Unknown function referenced");
    }
    if (calleeF->arg_size() != node.getArgs().size()) {
      return logError("Incorrect number of arguments passed");
    }
    std::vector<Value*> args;
    for (auto& arg : node.getArgs()) {
      args.push_back(md::visit(*this, *arg));
      if (!args.back()) {
        return nullptr;
      }
    }
    return builder.CreateCall(calleeF, args, "calltmp");
  }
  Value* operator()(IfExprAST& node) {
    Value* Cond = md::visit(*this, node.getCond());
    if (!Cond) {
      return nullptr;
    }

    Cond = builder.CreateFCmpONE(Cond, ConstantFP::get(context, APFloat(0.0)), "ifcond");

    Function* f = builder.GetInsertBlock()->getParent();

    BasicBlock* ThenBB = BasicBlock::Create(context, "then", f);
    BasicBlock* ElseBB = BasicBlock::Create(context, "else");
    BasicBlock* MergeBB = BasicBlock::Create(context, "ifcont");

    builder.CreateCondBr(Cond, ThenBB, ElseBB);

    builder.SetInsertPoint(ThenBB);

    Value* Then = md::visit(*this, node.getThen());
    if (!Then) {
      return nullptr;
    }

    builder.CreateBr(MergeBB);
    ThenBB = builder.GetInsertBlock();

    f->getBasicBlockList().push_back(ElseBB);
    builder.SetInsertPoint(ElseBB);

    Value* Else = md::visit(*this, node.getElse());
    if (!Else) {
      return nullptr;
    }

    builder.CreateBr(MergeBB);
    ElseBB = builder.GetInsertBlock();

    f->getBasicBlockList().push_back(MergeBB);
    builder.SetInsertPoint(MergeBB);
    PHINode* phi = builder.CreatePHI(Type::getDoubleTy(context), 2, "iftmp");

    phi->addIncoming(Then, ThenBB);
    phi->addIncoming(Else, ElseBB);

    return phi;
  }
  Value* operator()(ForExprAST& node) {
    Function* f = builder.GetInsertBlock()->getParent();

    AllocaInst* Alloca = CreateEntryBlockAlloca(f, node.getVarName());

    Value* Start = md::visit(*this, node.getStart());
    if (!Start) {
      return nullptr;
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

    Value* Body = md::visit(*this, node.getBody());
    if (!Body) {
      return nullptr;
    }

    Value* Step = nullptr;
    if (node.getStep()) {
      Step = md::visit(*this, node.getStep()->get());
      if (!Step) {
        return nullptr;
      }
    } else {
      Step = ConstantFP::get(context, APFloat(1.0));
    }

    Value* End = md::visit(*this, node.getEnd());
    if (!End) {
      return nullptr;
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

    return Constant::getNullValue(Type::getDoubleTy(context));
  }
  Value* operator()(VarExprAST& node) {
    std::vector<AllocaInst*> OldBindings;

    Function* f = builder.GetInsertBlock()->getParent();

    for (auto& entry : node.getVarNames()) {
      std::string const& VarName = entry.first;
      ExprAST* Init = entry.second.get();

      Value* InitVal;
      if (Init) {
        InitVal = md::visit(*this, *Init);
        if (!InitVal) {
          return nullptr;
        }
      } else {
        InitVal = ConstantFP::get(context, APFloat(0.0));
      }

      AllocaInst* Alloca = CreateEntryBlockAlloca(f, VarName);
      builder.CreateStore(InitVal, Alloca);
      OldBindings.push_back(namedValues[VarName]);
      namedValues[VarName] = Alloca;
    }

    Value* BodyVal = md::visit(*this, node.getBody());
    if (!BodyVal) {
      return nullptr;
    }

    auto it = OldBindings.begin();
    for (auto& entry : node.getVarNames()) {
      namedValues[entry.first] = *it++;
    }

    return BodyVal;
  }
  Function* operator()(PrototypeAST& node) {
    auto const& args = node.getArgs();
    std::vector<Type*> doubles(args.size(), Type::getDoubleTy(context));
    FunctionType* ft = FunctionType::get(Type::getDoubleTy(context), doubles, false);
    Function* f = Function::Create(ft, Function::ExternalLinkage, node.getName(), module.get());

    assert(f->arg_size() == args.size());
    auto it = args.begin();
    for (auto& arg : f->args()) {
      arg.setName(*it++);
    }

    return f;
  }
  Function* operator()(FunctionAST& node) {
    Function* f = module->getFunction(node.getPrototype().getName());

    if (!f) {
      f = static_cast<Function*>(md::visit(*this, node.getPrototype()));
    }
    if (!f) {
      return nullptr;
    }
    if (!f->empty()) {
      return logErrorF("Function cannot be redefined");
    }
    BasicBlock* bb = BasicBlock::Create(context, "entry", f);
    builder.SetInsertPoint(bb);

    namedValues.clear();
    for (auto& arg : f->args()) {
      AllocaInst* Alloca = CreateEntryBlockAlloca(f, arg.getName());
      builder.CreateStore(&arg, Alloca);
      namedValues[arg.getName()] = Alloca;
    }

    Value* retVal = md::visit(*this, node.getBody());
    if (retVal) {
      builder.CreateRet(retVal);
      llvm::verifyFunction(*f, &llvm::errs());
      fpm->run(*f);
      return f;
    }

    f->eraseFromParent();
    return f;
  }
};

#endif
