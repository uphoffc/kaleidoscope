#ifndef K_AST_H_
#define K_AST_H_

#include <memory>
#include <utility>
#include <string>
#include <vector>
#include <optional>

#include <md/type.hpp>

// Sort from derived to base classes
using ast_type = md::type< class NumberExprAST,
                           class VariableExprAST,
                           class BinaryExprAST,
                           class CallExprAST,
                           class IfExprAST,
                           class ForExprAST,
                           class VarExprAST,
                           class ExprAST,
                           class PrototypeAST,
                           class FunctionAST>;

class ExprAST : public ast_type {
public:
  virtual ~ExprAST() {}
};

class NumberExprAST : public md::with_type<NumberExprAST,ExprAST> {
private:
  double val;

public:
  NumberExprAST(double val)
    : val(val) {}

  double getNumber() const { return val; }
};

class VariableExprAST : public md::with_type<VariableExprAST,ExprAST> {
private:
  std::string name;

public:
  VariableExprAST(std::string const& name)
    : name(name) {}

  std::string const& getName() const { return name; }
};

class BinaryExprAST : public md::with_type<BinaryExprAST,ExprAST> {
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

class CallExprAST : public md::with_type<CallExprAST,ExprAST> {
private:
  std::string callee;
  std::vector<std::unique_ptr<ExprAST>> args;

public:
  CallExprAST(std::string const& callee, std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), args(std::move(args)) {}

  auto const& getArgs() const { return args; }

  std::string const& getCallee() const { return callee; }
};

class IfExprAST : public md::with_type<IfExprAST,ExprAST> {
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

class ForExprAST : public md::with_type<ForExprAST,ExprAST> {
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

class VarExprAST : public md::with_type<VarExprAST,ExprAST> {
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

class PrototypeAST : public md::with_type<PrototypeAST,ast_type>{
private:
  std::string name;
  std::vector<std::string> args;

public:
  PrototypeAST(std::string const& name, std::vector<std::string> args)
    : name(name), args(std::move(args)) {}

  std::string const& getName() const { return name; }

  auto const& getArgs() const { return args; }
};

class FunctionAST : public md::with_type<FunctionAST,ast_type> {
private:
  std::unique_ptr<PrototypeAST> proto;
  std::unique_ptr<ExprAST> body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> proto, std::unique_ptr<ExprAST> body)
    : proto(std::move(proto)), body(std::move(body)) {}

  PrototypeAST& getPrototype() {
    return *proto;
  }

  ExprAST& getBody() {
    return *body;
  }
};

#endif
