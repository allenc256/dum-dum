#include <bitset>
#include <iostream>
#include <random>

#include "State.h"

int main() {
  std::default_random_engine random;
  for (int i = 0; i < 1; i++) {
    State s = State::random(random);
    std::cout << s << std::endl;
  }
  return 0;
}
