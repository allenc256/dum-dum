#pragma once

#include <sstream>
#include <string>

template <class T> T from_string(std::string s) {
  std::istringstream is(s);
  T x;
  is >> x;
  return x;
}

template <class T> std::string to_string(T x) {
  std::ostringstream os;
  os << x;
  return os.str();
}