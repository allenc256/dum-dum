#include <bitset>
#include <iostream>
#include <random>

#include "Card.h"

int main(int argc, char **argv) {
  for (int i = 0; i < 1; i++) {
    GameState s = GameState::random();
    std::cout << s << std::endl;
  }
  return 0;
}
