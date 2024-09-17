#include "game_model.h"
#include "game_solver.h"
#include <bitset>
#include <iostream>
#include <random>

int main() {
  std::default_random_engine random(1234);
  Game g = Game::random_deal(random, 5);
  GameSolver gs = GameSolver(g);

  std::cout << g << std::endl;

  int best_tricks_by_ns = gs.solve();
  std::cout << "best_tricks_by_ns:  " << best_tricks_by_ns << std::endl;
  std::cout << "states_explored:    " << gs.states_explored() << std::endl;

  return 0;
}
