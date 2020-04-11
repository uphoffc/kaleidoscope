#ifndef K_AST_H_
#define K_AST_H_

#include <memory>
#include <utility>
#include <string>
#include <vector>
#include <optional>

#include "Visitor.h"

// Sort from derived to base classes
using ASTVisitor = Visitor<class NumberExprAST,
                           class VariableExprAST,
                           class BinaryExprAST,
                           class CallExprAST,
                           class IfExprAST,
                           class ForExprAST,
                           class VarExprAST,
                           class ExprAST,
                           class PrototypeAST,
                           class FunctionAST>;
using DefaultASTVisitor = DefaultVisitor<ASTVisitor>;

class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual void accept(ASTVisitor& visitor) = 0;
};

class NumberExprAST : public Visitable<NumberExprAST,ExprAST,ASTVisitor> {
private:
  double val;

public:
  NumberExprAST(double val)
    : val(val) {}

  double getNumber() const { return val; }
};

class VariableExprAST : public Visitable<VariableExprAST,ExprAST,ASTVisitor> {
private:
  std::string name;

public:
  VariableExprAST(std::string const& name)
    : name(name) {}

  std::string const& getName() const { return name; }
};

class BinaryExprAST : public Visitable<BinaryExprAST,ExprAST,ASTVisitor> {
private:
  char op;
  std::unique_ptr<ExprAST> lhs, rhs;

public:
  BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

  char getOp() const { return op; }
  ExprAST& getLHS() { return *lhs; }
  ExprAST& getRHS() { return *rhs; }
};

class CallExprAST : public Visitable<CallExprAST,ExprAST,ASTVisitor> {
private:
  std::string callee;
  std::vector<std::unique_ptr<ExprAST>> args;

public:
  CallExprAST(std::string const& callee, std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), args(std::move(args)) {}

  auto const& getArgs() const { return args; }

  std::string const& getCallee() const { return callee; }
};

class IfExprAST : public Visitable<IfExprAST,ExprAST,ASTVisitor> {
private:
  std::unique_ptr<ExprAST> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond,
            std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
    : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else))
  {}

  ExprAST& getCond() { return *Cond; }
  ExprAST& getThen() { return *Then; }
  ExprAST& getElse() { return *Else; }
};

class ForExprAST : public Visitable<ForExprAST,ExprAST,ASTVisitor> {
private:
  std::string varName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
  ForExprAST(std::string const& varName,
             std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End,
             std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body)
    : varName(varName), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body))
  {}

  std::string const& getVarName() const { return varName; }
  ExprAST& getStart() { return *Start; }
  ExprAST& getEnd() { return *End; }
  std::optional<std::reference_wrapper<ExprAST>> getStep() {
    return Step ? std::optional<std::reference_wrapper<ExprAST>>{*Step} : std::nullopt;
  }
  ExprAST& getBody() { return *Body; }
};

class VarExprAST : public Visitable<VarExprAST,ExprAST,ASTVisitor> {
private:
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> varNames;
  std::unique_ptr<ExprAST> body;
public:
  VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> varNames,
             std::unique_ptr<ExprAST> body)
    : varNames(std::move(varNames)), body(std::move(body))
  {}

  auto const& getVarNames() const { return varNames; }
  ExprAST& getBody() { return *body; }
};

class PrototypeAST {
private:
  std::string name;
  std::vector<std::string> args;

public:
  PrototypeAST(std::string const& name, std::vector<std::string> args)
    : name(name), args(std::move(args)) {}

  void accept(ASTVisitor& visitor) {
    visitor.visit(*this);
  }

  std::string const& getName() const { return name; }

  auto const& getArgs() const { return args; }
};

class FunctionAST {
private:
  std::unique_ptr<PrototypeAST> proto;
  std::unique_ptr<ExprAST> body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> proto, std::unique_ptr<ExprAST> body)
    : proto(std::move(proto)), body(std::move(body)) {}

  void accept(ASTVisitor& visitor) {
    visitor.visit(*this);
  }

  PrototypeAST& getPrototype() {
    return *proto;
  }

  ExprAST& getBody() {
    return *body;
  }
};

#endif
