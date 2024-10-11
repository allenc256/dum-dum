#pragma once

#include "game_model.h"
#include "mini_solver.h"

#include <absl/container/flat_hash_map.h>
#include <array>
#include <vector>

class Solver {
public:
  struct Result {
    int     tricks_taken_by_ns;
    int     tricks_taken_by_ew;
    Card    best_play;
    int64_t states_explored;
    int64_t states_memoized;
  };

  Solver(Game g);
  ~Solver();

  void enable_all_optimizations(bool enabled) {
    ab_pruning_enabled_    = enabled;
    tpn_table_enabled_     = enabled;
    move_ordering_enabled_ = enabled;
    mini_solver_enabled_   = enabled;
  }

  void enable_ab_pruning(bool enabled) { ab_pruning_enabled_ = enabled; }
  void enable_tpn_table(bool enabled) { tpn_table_enabled_ = enabled; }
  void enable_move_ordering(bool enabled) { move_ordering_enabled_ = enabled; }
  void enable_mini_solver(bool enabled) { mini_solver_enabled_ = enabled; }

  void enable_tracing(std::ostream *os) {
    trace_ostream_ = os;
    trace_lineno_  = 0;
  }

  Game       &game() { return game_; }
  const Game &game() const { return game_; }

  Result solve();
  Result solve(int alpha, int beta);

private:
  Bounds compute_initial_bounds();

  int solve_internal(int alpha, int beta);

  struct SearchState {
    Cards ignorable;
    Cards already_searched;
    bool  maximizing;
    int   alpha;
    int   beta;
    int   best_tricks_by_ns;
  };

  enum Order { LOW_TO_HIGH, HIGH_TO_LOW };

  bool search_all_cards(SearchState &s);
  bool search_specific_cards(SearchState &s, Cards c, Order o);
  bool search_specific_card(SearchState &s, Card c);

  void trace(const char *tag, int alpha, int beta, int tricks_taken_by_ns);

  Game          game_;
  int           search_ply_;
  Card          best_play_;
  int64_t       states_explored_;
  TpnTable      tpn_table_;
  MiniSolver    mini_solver_;
  bool          ab_pruning_enabled_;
  bool          tpn_table_enabled_;
  bool          move_ordering_enabled_;
  bool          mini_solver_enabled_;
  std::ostream *trace_ostream_;
  int64_t       trace_lineno_;
};
