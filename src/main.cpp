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

  g.hands().pretty_print(std::cout, Cards::all());
  std::cout << '\n';
  g.hands().pretty_print(std::cout, r.winners_by_rank);
  std::cout << '\n';
  std::cout << "trump_suit         " << g.trump_suit() << '\n'
            << "next_seat          " << g.next_seat() << '\n'
            << "best_tricks_by_ns  " << r.tricks_taken_by_ns << '\n'
            << "best_tricks_by_ew  " << r.tricks_taken_by_ew << '\n'
            << "states_explored:   " << s.states_explored() << '\n'
            << "states_memoized:   " << s.states_memoized() << '\n'
            << "elapsed_ms:        " << elapsed_ms << std::endl;
}

int main() {
  solve_seed(123, 8);
  return 0;
}
