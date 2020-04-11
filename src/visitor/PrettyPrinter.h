#ifndef K_VISITOR_PRETTYPRINTER_H_
#define K_VISITOR_PRETTYPRINTER_H_

#include <iostream>
#include <sstream>

#include "Visitor.h"
#include "AST.h"

class PrettyPrinter : public DefaultASTVisitor {
private:
  int level = 0;

  template<typename T>
  void print(T const& t) {
    for (int i = 0; i < level; ++i) {
      std::cout << "  ";
    }
    std::cout << t << std::endl;
  }

public:
  void visit(NumberExprAST& node) override {
    print(node.getNumber());
  }
  void visit(VariableExprAST& node) override {
    print(node.getName());
  }
  void visit(BinaryExprAST& node) override {
    print(node.getOp());
    ++level;
    node.getLHS().accept(*this);
    node.getRHS().accept(*this);
    --level;
  }
  void visit(CallExprAST& node) override {
    print("call " + node.getCallee());
    ++level;
    for (auto& arg : node.getArgs()) {
      arg->accept(*this);
    }
    --level;
  }
  void visit(IfExprAST& node) override {
    print("if");
    ++level;
    node.getCond().accept(*this);
    --level;
    print("then");
    ++level;
    node.getThen().accept(*this);
    --level;
    print("else");
    ++level;
    node.getElse().accept(*this);
    --level;
  }
  void visit(ForExprAST& node) override {
    print("for");
    ++level;
    print("start");
    ++level;
    node.getStart().accept(*this);
    --level;
    print("end");
    ++level;
    node.getEnd().accept(*this);
    --level;
    if (node.getStep()) {
      print("step");
      ++level;
      node.getStep()->get().accept(*this);
      --level;
    }
    --level;
    print("in");
    ++level;
    node.getBody().accept(*this);
    --level;
  }
  void visit(PrototypeAST& node) override {
    std::stringstream ss;
    ss << "def " << node.getName() << " ( ";
    for (auto& child : node.getArgs()) {
      ss << child << " ";
    }
    ss << ")";
    print(ss.str());
  }
  void visit(FunctionAST& node) override {
    node.getPrototype().accept(*this);
    ++level;
    node.getBody().accept(*this);
    --level;
  }
};

#endif
