#pragma once

#include "game_model.h"
#include "mini_solver.h"

#include <absl/container/flat_hash_map.h>
#include <array>
#include <ostream>
#include <vector>

class PlayOrder {
public:
  static constexpr bool LOW_TO_HIGH = true;
  static constexpr bool HIGH_TO_LOW = false;

  PlayOrder() : card_count_(0) {}

  const Card *begin() const { return cards_; }
  const Card *end() const { return cards_ + card_count_; }

  void append_plays(Cards cards, bool low_to_high);

private:
  friend class Iter;

  Cards  all_cards_;
  Card   cards_[13];
  int8_t card_count_;
};

class Solver {
public:
  struct Result {
    int   tricks_taken_by_ns;
    int   tricks_taken_by_ew;
    Card  best_play;
    Cards winners_by_rank;
  };

  Solver(Game g);
  ~Solver();

  int64_t states_explored() const { return states_explored_; }
  int64_t states_memoized() const { return tpn_table_.size(); }

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
  Result solve(int alpha, int beta, int max_depth);

private:
  int solve_internal(
      int alpha, int beta, int max_depth, Cards &winners_by_rank
  );

  void lookup_tpn_value(int max_depth, TpnTable::Value &value);
  void order_plays(PlayOrder &order) const;

  void search_all_cards(
      int    max_depth,
      int    alpha,
      int    beta,
      int   &best_score,
      Card  &best_play,
      Cards &winners_by_rank
  );

  void trace(const char *tag, int alpha, int beta, int tricks_taken_by_ns);

  Game                game_;
  int                 search_ply_;
  std::optional<Card> best_play_;
  int64_t             states_explored_;
  TpnTable            tpn_table_;
  MiniSolver          mini_solver_;
  bool                ab_pruning_enabled_;
  bool                tpn_table_enabled_;
  bool                move_ordering_enabled_;
  bool                mini_solver_enabled_;
  std::ostream       *trace_ostream_;
  int64_t             trace_lineno_;
};
