#include <iostream>
#include <sstream>
#include "Lexer.h"
#include "Parser.h"
#include "visitor/PrettyPrinter.h"
#include "visitor/CodeGen.h"

#include "llvm/Support/TargetSelect.h"

static void HandleDefinition(Parser& parser) {
  if (parser.parseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    // Skip token for error recovery.
    parser.getNextToken();
  }
}

static void HandleExtern(Parser& parser) {
  if (parser.parseExtern()) {
    fprintf(stderr, "Parsed an extern\n");
  } else {
    // Skip token for error recovery.
    parser.getNextToken();
  }
}

static void HandleTopLevelExpression(Parser& parser) {
  // Evaluate a top-level expression into an anonymous function.
  if (parser.parseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expr\n");
  } else {
    // Skip token for error recovery.
    parser.getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
static void MainLoop(Parser& parser) {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (parser.curTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      parser.getNextToken();
      break;
    case tok_def:
      HandleDefinition(parser);
      break;
    case tok_extern:
      HandleExtern(parser);
      break;
    default:
      HandleTopLevelExpression(parser);
      break;
    }
  }
}

int main() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  //std::string code("def test(x y z) x+y*5.0-z*z*z * test(x,y,z);");
  //std::string code("def test(x) if x < 5 then (1+2+x)*(x+(1+2)) else x+x;");
  std::string code("def test(n) var x=5,y=6 in x*n+y*n;");
  //std::string code("def text(x y) x + y;");
  std::stringstream codeStream(code);
  Lexer lexer(codeStream);
  Parser parser(lexer);
  parser.getNextToken();
  auto ast = parser.parseDefinition();
  PrettyPrinter pp;
  ast->accept(pp);

  CodeGen cg;
  ast->accept(cg);

  for (auto& val : cg.getFunctions()) {
    val->print(llvm::errs());
    std::cerr << std::endl;
  }

  return 0;
}
