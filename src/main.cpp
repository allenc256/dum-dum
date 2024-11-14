#include "game_model.h"
#include "random.h"
#include "solver.h"

#include <chrono>
#include <iostream>

void solve_seed(int seed, int cards_per_hand) {
  Game   g = Random(seed).random_game(cards_per_hand);
  Solver s(g);

  auto begin = std::chrono::steady_clock::now();
  auto r     = s.solve();
  auto end   = std::chrono::steady_clock::now();
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();

  auto  stats     = s.stats();
  auto &tpn_stats = stats.tpn_table_stats;

  g.hands().pretty_print(std::cout, Cards::all());
  std::cout << '\n';
  g.hands().pretty_print(std::cout, r.winners_by_rank);
  std::cout << '\n';
  std::cout << "trump_suit         " << g.trump_suit() << '\n';
  std::cout << "next_seat          " << g.next_seat() << '\n';
  std::cout << "best_tricks_by_ns  " << r.tricks_taken_by_ns << '\n';
  std::cout << "best_tricks_by_ew  " << r.tricks_taken_by_ew << '\n';
  std::cout << "nodes_explored     " << stats.nodes_explored << '\n';
  std::cout << "tpn_buckets        " << tpn_stats.buckets << '\n';
  std::cout << "tpn_entries        " << tpn_stats.entries << '\n';
  std::cout << "tpn_lookup_hits    " << tpn_stats.lookup_hits << '\n';
  std::cout << "tpn_lookup_misses  " << tpn_stats.lookup_misses << '\n';
  std::cout << "tpn_lookup_reads   " << tpn_stats.lookup_reads << '\n';
  std::cout << "tpn_insert_hits    " << tpn_stats.insert_hits << '\n';
  std::cout << "tpn_insert_misses  " << tpn_stats.insert_misses << '\n';
  std::cout << "tpn_insert_reads   " << tpn_stats.insert_reads << '\n';
  std::cout << "elapsed_ms         " << elapsed_ms << std::endl;
}

int main() {
  solve_seed(123, 8);
  return 0;
}
