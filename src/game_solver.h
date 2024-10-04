#pragma once

#include "game_mini_solver.h"
#include "game_model.h"

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
    int64_t ms_states_memoized;
  };

  Solver(Game g);
  ~Solver();

  void enable_all_optimizations(bool enabled) {
    ab_pruning_enabled_    = enabled;
    tp_table_enabled_      = enabled;
    tp_table_norm_enabled_ = enabled;
    move_ordering_enabled_ = enabled;
    mini_solver_enabled_   = enabled;
  }

  void enable_ab_pruning(bool enabled) { ab_pruning_enabled_ = enabled; }
  void enable_tp_table(bool enabled) { tp_table_enabled_ = enabled; }
  void enable_tp_table_norm(bool enabled) { tp_table_norm_enabled_ = enabled; }
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
  int solve_internal(int alpha, int beta, Card *best_play);

  struct SearchState {
    Cards ignorable;
    Cards already_searched;
    bool  maximizing;
    int   alpha;
    int   beta;
    int   best_tricks_by_ns;
    Card *best_play;
  };

  enum Order { LOW_TO_HIGH, HIGH_TO_LOW };

  bool search_all_cards(SearchState &s);
  bool search_specific_cards(SearchState &s, Cards c, Order o);
  bool search_specific_card(SearchState &s, Card c);
  int  search_forced_tricks(bool maximizing, int &alpha, int &beta);

  void trace(
      const char      *tag,
      const GameState *state,
      int              alpha,
      int              beta,
      int              tricks_taken_by_ns
  );

  struct Bounds {
    int8_t lower;
    int8_t upper;
  };

  typedef absl::flat_hash_map<GameState, Bounds> TranspositionTable;

  Game               game_;
  int64_t            states_explored_;
  TranspositionTable tp_table_;
  MiniSolver         mini_solver_;
  bool               ab_pruning_enabled_;
  bool               tp_table_enabled_;
  bool               tp_table_norm_enabled_;
  bool               move_ordering_enabled_;
  bool               mini_solver_enabled_;
  std::ostream      *trace_ostream_;
  int64_t            trace_lineno_;
};
