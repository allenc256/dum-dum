#include <bitset>
#include <iostream>
#include <random>

#include "Game.h"

int main() {
  std::default_random_engine random(123);
  Game g = Game::random_deal(random);

  std::cout << g << std::endl;
  g.play(Card(RANK_5, CLUBS));
  g.play(Card(JACK, CLUBS));

  std::cout << g << std::endl;

  return 0;
}
