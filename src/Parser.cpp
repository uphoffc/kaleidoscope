#include "Parser.h"
#include <cctype>
#include <iostream>

std::unique_ptr<ExprAST> logError(char const* str) {
  std::cerr << "Error: " << str << std::endl;
  return nullptr;
}

std::unique_ptr<PrototypeAST> logErrorP(char const* str) {
  logError(str);
  return nullptr;
}

const std::unordered_map<char, int> Parser::BinopPrecedence{
  {'=', 2},
  {'<', 10},
  {'+', 20},
  {'-', 20},
  {'*', 40}
};

int Parser::getTokPrecedence() {
  if (!isascii(curTok)) {
    return -1;
  }

  auto tokPrec = BinopPrecedence.find(curTok);
  if (tokPrec == BinopPrecedence.end()) {
    return -1;
  }
  return tokPrec->second;
}

std::unique_ptr<ExprAST> Parser::parseNumberExpr() {
  auto result = std::make_unique<NumberExprAST>(lexer.getNumericValue());
  getNextToken();
  return std::move(result);
}

std::unique_ptr<ExprAST> Parser::parseParenExpr() {
  getNextToken(); // skip '('
  auto expr = parseExpression();
  if (!expr) {
    return nullptr;
  }

  if (curTok != ')') {
    return logError("expected ')'");
  }
  getNextToken(); // skip ')'
  return expr;
}

std::unique_ptr<ExprAST> Parser::parseIdentifierExpr() {
  std::string identifier = lexer.getIdentifier();
  
  getNextToken(); // skip identifier

  // Variable
  if (curTok != '(') {
    return std::make_unique<VariableExprAST>(identifier);
  }

  // Function call
  getNextToken(); // skip '('
  std::vector<std::unique_ptr<ExprAST>> args;
  if (curTok != ')') {
    while (true) {
      if (auto arg = parseExpression()) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }

      if (curTok == ')') {
        break;
      }

      if (curTok != ',') {
        return logError("Expected ')' or ',' in argument list");
      }
      getNextToken();
    }
  }
  getNextToken(); // skip ')'

  return std::make_unique<CallExprAST>(identifier, std::move(args));
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
  switch (curTok) {
    default:
      return logError("Unknown token when expecting an expression");
    case tok_identifier:
      return parseIdentifierExpr();
    case tok_number:
      return parseNumberExpr();
    case '(':
      return parseParenExpr();
    case tok_if:
      return parseIfExpr();
    case tok_for:
      return parseForExpr();
    case tok_var:
      return parseVarExpr();
  }
}

std::unique_ptr<ExprAST> Parser::parseExpression() {
  auto lhs = parsePrimary();
  if (!lhs) {
    return nullptr;
  }

  return parseBinOpRHS(0, std::move(lhs));
}

std::unique_ptr<ExprAST> Parser::parseBinOpRHS(int minPrec, std::unique_ptr<ExprAST> lhs) {
  while (true) {
    int tokPrec = getTokPrecedence();

    if (tokPrec < minPrec) {
      return lhs;
    }

    int binOp = curTok;
    getNextToken();
    auto rhs = parsePrimary();
    if (!rhs) {
      return nullptr;
    }

    int nextPrec = getTokPrecedence();
    if (tokPrec < nextPrec) {
      rhs = parseBinOpRHS(tokPrec+1, std::move(rhs));
      if (!rhs) {
        return nullptr;
      }
    }
    lhs = std::make_unique<BinaryExprAST>(binOp, std::move(lhs), std::move(rhs));
  }
}

std::unique_ptr<ExprAST> Parser::parseIfExpr() {
  getNextToken(); // skip if

  auto Cond = parseExpression();
  if (!Cond) {
    return nullptr;
  }

  if (curTok != tok_then) {
    return logError("expected then");
  }
  getNextToken(); // skip then

  auto Then = parseExpression();
  if (!Then) {
    return nullptr;
  }

  if (curTok != tok_else) {
    return logError("expected else");
  }
  getNextToken();

  auto Else = parseExpression();
  if (!Else) {
    return nullptr;
  }

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
}

std::unique_ptr<ExprAST> Parser::parseForExpr() {
  getNextToken(); // skip for

  if (curTok != tok_identifier) {
    return logError("expected identifier after for");
  }

  std::string idName = lexer.getIdentifier();
  getNextToken(); // skip identifier

  if (curTok != '=') {
    return logError("Expected '=' after for");
  }
  getNextToken(); // skip '='

  auto Start = parseExpression();
  if (!Start) {
    return nullptr;
  }
  if (curTok != ',') {
    return logError("Expected ',' after for start value");
  }
  getNextToken();

  auto End = parseExpression();
  if (!End) {
    return nullptr;
  }

  std::unique_ptr<ExprAST> Step;
  if (curTok == ',') {
    getNextToken();
    Step = parseExpression();
    if (!Step) {
      return nullptr;
    }
  }

  if (curTok != tok_in) {
    return logError("expected 'in' after for");
  }
  getNextToken(); // skip in

  auto Body = parseExpression();
  if (!Body) {
    return nullptr;
  }

  return std::make_unique<ForExprAST>(idName, std::move(Start),
                                              std::move(End),
                                              std::move(Step),
                                              std::move(Body));
}

std::unique_ptr<ExprAST> Parser::parseVarExpr() {
  getNextToken(); // skip var
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> varNames;

  if (curTok != tok_identifier) {
    return logError("expected identifier after var");
  }

  while (true) {
    std::string name = lexer.getIdentifier();
    getNextToken(); // skip identifier

    std::unique_ptr<ExprAST> init;
    if (curTok == '=') {
      getNextToken(); // skip '='

      init = parseExpression();
      if (!init) {
        return nullptr;
      }
    }

    varNames.push_back(std::make_pair(name, std::move(init)));

    if (curTok != ',') {
      break;
    }
    getNextToken(); // skip ','

    if (curTok != tok_identifier) {
      return logError("expected identifier after var");
    }
  }

  if (curTok != tok_in) {
    return logError("Expected 'in' keyword after 'var'");
  }
  getNextToken(); // skip in

  auto body = parseExpression();
  if (!body) {
    return nullptr;
  }

  return std::make_unique<VarExprAST>(std::move(varNames), std::move(body));
}

std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
  if (curTok != tok_identifier) {
    return logErrorP("Expected function name in prototype");
  }

  std::string fnName = lexer.getIdentifier();
  getNextToken();

  if (curTok != '(') {
    return logErrorP("Expected '(' in prototype");
  }

  std::vector<std::string> argNames;
  while (getNextToken() == tok_identifier) {
    argNames.push_back(lexer.getIdentifier());
  }
  if (curTok != ')') {
    return logErrorP("Expected ')' in prototype");
  }

  getNextToken(); // skip ')'

  return std::make_unique<PrototypeAST>(fnName, std::move(argNames));
}

std::unique_ptr<FunctionAST> Parser::parseDefinition() {
  getNextToken();
  auto proto = parsePrototype();
  if (!proto) {
    return nullptr;
  }

  if (auto e = parseExpression()) {
    return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
  }
  return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::parseExtern() {
  getNextToken();
  return parsePrototype();
}

std::unique_ptr<FunctionAST> Parser::parseTopLevelExpr() {
  if (auto e = parseExpression()) {
    auto proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
  }
  return nullptr;
}
