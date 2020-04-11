#ifndef K_PARSER_H_
#define K_PARSER_H_

#include <memory>
#include <unordered_map>
#include "Lexer.h"
#include "AST.h"

class Parser {
//private:
public:
  int curTok;
  Lexer& lexer;

  int getTokPrecedence();

public:
  static const std::unordered_map<char, int> BinopPrecedence;

  Parser(Lexer& lexer)
    : lexer(lexer) {}

  int getNextToken() {
    return curTok = lexer.getToken();
  }

  std::unique_ptr<ExprAST> parseNumberExpr();
  std::unique_ptr<ExprAST> parseParenExpr();
  std::unique_ptr<ExprAST> parseIdentifierExpr();
  std::unique_ptr<ExprAST> parsePrimary();
  std::unique_ptr<ExprAST> parseExpression();
  std::unique_ptr<ExprAST> parseBinOpRHS(int minPrec, std::unique_ptr<ExprAST> lhs);
  std::unique_ptr<ExprAST> parseIfExpr();
  std::unique_ptr<ExprAST> parseForExpr();
  std::unique_ptr<ExprAST> parseVarExpr();
  std::unique_ptr<PrototypeAST> parsePrototype();
  std::unique_ptr<FunctionAST> parseDefinition();
  std::unique_ptr<PrototypeAST> parseExtern();
  std::unique_ptr<FunctionAST> parseTopLevelExpr();
};

#endif
