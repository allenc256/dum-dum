#include "game_model.h"
#include "game_solver.h"
#include <bitset>
#include <fstream>
#include <iostream>
#include <random>

void show_line(const Game &g, std::string line) {
  std::istringstream is(line);
  Game copy(g);

  std::cout << "playing: ";
  while (is.peek() != EOF) {
    Card c;
    is >> c;
    std::cout << c;
    copy.play(c);
  }
  std::cout << std::endl;
  std::cout << copy;
}

int main() {
  std::default_random_engine random(123);

  for (int i = 0; i < 6; i++) {
    Game g = Game::random_deal(random, 3);
    if (i != 5) {
      continue;
    }

    Solver s1(g);
    Solver s2(g);
    s1.enable_all_optimizations(false);
    s1.enable_transposition_table(true);
    s2.enable_all_optimizations(false);

    for (int i = 0; i < 5; i++) {
      auto r1 = s1.solve();
      auto r2 = s2.solve();
      s1.game().play(r1.best_play());
      s2.game().play(r1.best_play());
      std::cout << r1.tricks_taken_by_ns() << " " << r2.tricks_taken_by_ns()
                << std::endl;
    }
  }

  return 0;
}
