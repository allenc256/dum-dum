#include <format>

#include "parser.h"

Parser::Parser(std::string_view str) : str_(str), remaining_(str) {}

bool Parser::try_parse(std::string_view next) {
  if (remaining_.starts_with(next)) {
    remaining_ = remaining_.substr(next.length());
    return true;
  } else {
    return false;
  }
}

bool Parser::try_parse(char next) {
  if (remaining_.starts_with(next)) {
    remaining_ = remaining_.substr(1);
    return true;
  } else {
    return false;
  }
}

bool Parser::finished() const { return remaining_.empty(); }

Parser::Error Parser::error(std::string_view message) const {
  std::string err_loc;
  std::size_t parsed = str_.size() - remaining_.size();

  for (std::size_t i = 0; i < parsed; i++) {
    err_loc += ' ';
  }
  err_loc += '^';

  throw Error(std::format("parsing error: {}\n{}\n{}", message, str_, err_loc));
}
