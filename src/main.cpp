#include "game_model.h"
#include "random.h"
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
    Game   g = Random(i).random_game(cards_per_hand);
    Solver s(g);

    auto begin = std::chrono::steady_clock::now();
    auto r     = s.solve();
    auto end   = std::chrono::steady_clock::now();
    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();

    std::printf(
        "%20d,%20d,%20lld,%20lld,%20lld\n",
        i,
        r.tricks_taken_by_ns,
        s.states_explored(),
        s.states_memoized(),
        elapsed_ms
    );
  }
}

void solve_seed(int seed, int cards_per_hand) {
  Game   g = Random(seed).random_game(cards_per_hand);
  Solver s(g);

  auto begin = std::chrono::steady_clock::now();
  auto r     = s.solve();
  auto end   = std::chrono::steady_clock::now();
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();
  auto &stats = s.tpn_table_stats();

  std::cout << g << std::endl;
  std::cout << "best_tricks_by_ns:  " << r.tricks_taken_by_ns << '\n'
            << "states_explored:    " << s.states_explored() << '\n'
            << "mini_explored:      " << s.mini_solver_states_explored() << '\n'
            << "lookup_examined:    " << stats.lookup_entries_examined << '\n'
            << "lookup_misses:      " << stats.lookup_misses << '\n'
            << "lookup_hits:        " << stats.lookup_hits << '\n'
            << "upsert_examined:    " << stats.upsert_entries_examined << '\n'
            << "upsert_misses:      " << stats.upsert_misses << '\n'
            << "upsert_hits:        " << stats.upsert_hits << '\n'
            << "max_bucket_size:    " << stats.max_bucket_size << '\n'
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
