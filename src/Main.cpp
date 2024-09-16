#include "game_model.h"
#include <bitset>
#include <iostream>
#include <random>

int main() {
  std::default_random_engine random(123);
  Game g = Game::random_deal(random, 5);

  std::cout << g << std::endl;
  g.play(Card(RANK_7, CLUBS));
  g.play(Card(QUEEN, CLUBS));
  g.play(Card(RANK_5, CLUBS));
  g.play(Card(JACK, CLUBS));
  std::cout << g << std::endl;

  g.play(Card(KING, CLUBS));
  std::cout << g << std::endl;

  return 0;
}
