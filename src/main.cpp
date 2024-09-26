#include "game_model.h"
#include "game_solver.h"
#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>

void show_line(const Game &g, std::string line) {
  std::istringstream is(line);
  Game               copy(g);

  std::cout << copy << std::endl;
  while (is.peek() != EOF) {
    Card c;
    is >> c;
    std::cout << g.next_seat() << " plays " << c << std::endl << std::endl;
    copy.play(c);
    std::cout << copy << std::endl;
  }
}

int main() {
  std::default_random_engine random(123);
  Game                       g = Game::random_deal(random, 13);
  Solver                     s(g);

  auto begin = std::chrono::steady_clock::now();
  auto r     = s.solve();
  auto end   = std::chrono::steady_clock::now();
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();

  std::cout << g << std::endl;
  std::cout << "best_tricks_by_ns:  " << r.tricks_taken_by_ns() << std::endl
            << "states_explored:    " << r.states_explored() << std::endl
            << "states_memoized:    " << r.states_memoized() << std::endl
            << "elapsed_ms:         " << elapsed_ms << std::endl;

  while (!s.game().finished()) {
    s.game().play(r.best_play());
    r = s.solve();
  }
  std::cout << std::left;
  for (int i = 0; i < s.game().tricks_taken(); i++) {
    std::cout << std::setw(20) << std::format("trick_{}:", i)
              << s.game().trick(i) << std::endl;
  }

  return 0;
}
