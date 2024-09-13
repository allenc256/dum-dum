#include <bitset>
#include <iostream>
#include <random>

#include "State.h"

int main() {
  for (int i = 0; i < 1; i++) {
    State s = State::random();
    std::cout << s << std::endl;
  }
  return 0;
}
