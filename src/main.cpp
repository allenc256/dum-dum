#include "game_model.h"
#include "solver.h"

#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>

void benchmark(int num_games, int cards_per_hand) {
  std::printf(
      "%20s,%20s,%20s,%20s,%20s\n",
      "seed",
      "tricks_taken_by_ns",
      "states_explored",
      "states_memoized",
      "elapsed_ms"
  );

  for (int i = 0; i < num_games; i++) {
    std::default_random_engine random(i);
    Game                       g = Game::random_deal(random, cards_per_hand);
    Solver                     s(g);

    auto begin = std::chrono::steady_clock::now();
    auto r     = s.solve();
    auto end   = std::chrono::steady_clock::now();
    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();

    std::printf(
        "%20d,%20d,%20ld,%20ld,%20ld\n",
        i,
        r.tricks_taken_by_ns,
        r.states_explored,
        r.states_memoized,
        elapsed_ms
    );
  }
}

void solve_seed(int seed, int cards_per_hand) {
  std::default_random_engine random(seed);
  Game                       g = Game::random_deal(random, cards_per_hand);
  Solver                     s(g);

  auto begin = std::chrono::steady_clock::now();
  auto r     = s.solve();
  auto end   = std::chrono::steady_clock::now();
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();

  std::cout << g << std::endl;
  std::cout << "best_tricks_by_ns:  " << r.tricks_taken_by_ns << std::endl
            << "states_explored:    " << r.states_explored << std::endl
            << "states_memoized:    " << r.states_memoized << std::endl
            << "elapsed_ms:         " << elapsed_ms << std::endl;

  while (!s.game().finished()) {
    s.game().play(r.best_play);
    r = s.solve();
  }
  std::cout << std::left;
  char buf[256];
  for (int i = 0; i < s.game().tricks_taken(); i++) {
    snprintf(buf, sizeof(buf), "trick_%d:", i);
    std::cout << std::setw(20) << buf << s.game().trick(i) << std::endl;
  }
}

int main() {
  solve_seed(123, 13);
  return 0;
}
