#ifndef K_LEXER_H_
#define K_LEXER_H_

#include <cctype>
#include <string>
#include <istream>
#include <unordered_map>

enum Token {
  tok_eof = -1,
  tok_def = -2,
  tok_extern = -3,
  tok_identifier = -4,
  tok_number = -5,
  tok_if = -6,
  tok_then = -7,
  tok_else = -8,
  tok_for = -9,
  tok_in = -10,
  tok_var = -11
};

class Lexer {
private:
  std::string identifier;
  double numericValue;
  char lastChar = ' ';
  std::istream& in;

public:
  explicit Lexer(std::istream& in)
    : in(in) {}

  int getToken();
  double getNumericValue() const { return numericValue; }
  std::string getIdentifier() const { return identifier; }
};

#endif
