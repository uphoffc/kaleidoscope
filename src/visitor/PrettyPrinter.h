#ifndef K_VISITOR_PRETTYPRINTER_H_
#define K_VISITOR_PRETTYPRINTER_H_

#include <iostream>
#include <sstream>

#include <md/visit.hpp>
#include "AST.h"

class PrettyPrinter {
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
  void operator()(ExprAST&) {}

  void operator()(NumberExprAST& node) {
    print(node.getNumber());
  }
  void operator()(VariableExprAST& node) {
    print(node.getName());
  }
  void operator()(BinaryExprAST& node) {
    print(node.getOp());
    ++level;
    md::visit(*this, node.getLHS());
    md::visit(*this, node.getRHS());
    --level;
  }
  void operator()(CallExprAST& node) {
    print("call " + node.getCallee());
    ++level;
    for (auto& arg : node.getArgs()) {
      md::visit(*this, *arg);
    }
    --level;
  }
  void operator()(IfExprAST& node) {
    print("if");
    ++level;
    md::visit(*this, node.getCond());
    --level;
    print("then");
    ++level;
    md::visit(*this, node.getThen());
    --level;
    print("else");
    ++level;
    md::visit(*this, node.getElse());
    --level;
  }
  void operator()(ForExprAST& node) {
    print("for");
    ++level;
    print("start");
    ++level;
    md::visit(*this, node.getStart());
    --level;
    print("end");
    ++level;
    md::visit(*this, node.getEnd());
    --level;
    if (node.getStep()) {
      print("step");
      ++level;
      md::visit(*this, node.getStep()->get());
      --level;
    }
    --level;
    print("in");
    ++level;
    md::visit(*this, node.getBody());
    --level;
  }
  void operator()(VarExprAST& node) {
    //print(node.getName());
  }
  void operator()(PrototypeAST& node) {
    std::stringstream ss;
    ss << "def " << node.getName() << " ( ";
    for (auto& child : node.getArgs()) {
      ss << child << " ";
    }
    ss << ")";
    print(ss.str());
  }
  void operator()(FunctionAST& node) {
    md::visit(*this, node.getPrototype());
    ++level;
    md::visit(*this, node.getBody());
    --level;
  }
};

#endif
