#include "Panic.h"

#include <iostream>

void panic(std::string_view msg) {
  std::cerr << msg << std::endl;
  exit(1);
}
