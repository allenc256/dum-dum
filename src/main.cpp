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
  Card               c;
  Seat               s;

  std::cout << copy << std::endl;
  while (is.peek() != EOF) {
    for (int i = 0; i < 4; i++) {
      is >> c;
      copy.play(c);
    }
    is >> s;
    std::cout << copy << std::endl;
  }
}

int main() {
  std::default_random_engine random(123);

  for (int i = 0; i < 5; i++) {
    Game::random_deal(random, 4);
  }

  Game g = Game::random_deal(random, 4);

  // std::cout << sizeof(Solver::State) << std::endl;
  // exit(1);

  // show_line(g, "3♠T♠8♠7♠ N 4♠J♦6♣K♥ E 6♦7♦4♥3♦ S 2♣2♥5♥4♦ E");
  // exit(1);

  std::cout << g << std::endl;

  Solver s2(g);
  s2.enable_all_optimizations(true);

  std::ofstream os("trace.txt");
  s2.enable_tracing(&os);
  auto r2 = s2.solve();
  s2.enable_tracing(nullptr);

  do {
    s2.game().play(r2.best_play());
    r2 = s2.solve();
  } while (!s2.game().finished());

  std::cout << "best_tricks_by_ns:  " << r2.tricks_taken_by_ns() << std::endl;
  std::cout << "best_line:         ";
  for (int i = 0; i < s2.game().tricks_taken(); i++) {
    std::cout << " " << s2.game().trick(i);
  }
  std::cout << std::endl;

  std::cout << std::endl;

  // auto begin = std::chrono::steady_clock::now();
  // auto r     = s.solve();
  // auto end   = std::chrono::steady_clock::now();
  // auto elapsed_ms =
  //     std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
  //         .count();

  // std::cout << g << std::endl;
  // std::cout << "best_tricks_by_ns:  " << r.tricks_taken_by_ns() << std::endl
  //           << "states_explored:    " << r.states_explored() << std::endl
  //           << "table_size:         " << r.transposition_table_size()
  //           << std::endl
  //           << "elapsed_ms:         " << elapsed_ms << std::endl
  //           << std::endl;

  // while (!s.game().finished()) {
  //   s.game().play(r.best_play());
  //   r = s.solve();
  // }

  // for (int i = 0; i < s.game().tricks_taken(); i++) {
  //   std::cout << s.game().trick(i) << std::endl;
  // }

  // return 0;
}
