#include "Lexer.h"
#include <sstream>

int Lexer::getToken() {
  while (isspace(lastChar)) {
    in.get(lastChar);
  }

  if (isalpha(lastChar)) {
    identifier.clear();
    do {
      identifier += lastChar;
      in.get(lastChar);
    } while (isalnum(lastChar));

    if (identifier == "def") {
      return tok_def;
    }
    if (identifier == "extern") {
      return tok_extern;
    }
    if (identifier == "if") {
      return tok_if;
    }
    if (identifier == "then") {
      return tok_then;
    }
    if (identifier == "else") {
      return tok_else;
    }
    if (identifier == "for") {
      return tok_for;
    }
    if (identifier == "in") {
      return tok_in;
    }
    if (identifier == "var") {
      return tok_var;
    }
    return tok_identifier;
  }

  if (isdigit(lastChar) || lastChar == '.') {
    std::stringstream value;
    do {
      value << lastChar;
      in.get(lastChar);
    } while (isdigit(lastChar) || lastChar == '.');
    value >> numericValue;
    return tok_number;
  }

  if (lastChar == '#') {
    do {
      in.get(lastChar);
    } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');

    if (lastChar != EOF) {
      return getToken();
    }
  }

  if (lastChar == EOF) {
    return tok_eof;
  }

  int thisChar = lastChar;
  in.get(lastChar);
  return thisChar;
}
