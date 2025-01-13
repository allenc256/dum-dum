#pragma once

#include <string_view>

class Parser {
public:
  class Error : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  Parser(std::string_view str);

  char  peek() const;
  bool  try_parse(std::string_view next);
  bool  try_parse(char next);
  bool  finished() const;
  Error error(std::string_view message) const;

private:
  std::string_view str_;
  std::string_view remaining_;
};
