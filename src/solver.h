#pragma once

#include "game_model.h"
#include "tpn_table.h"

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
    Cards winners_by_rank;
  };

  struct Stats {
    int64_t         nodes_explored;
    TpnTable::Stats tpn_table_stats;
  };

  Solver(Game g);
  ~Solver();

  Stats stats() const {
    return {
        .nodes_explored  = nodes_explored_,
        .tpn_table_stats = tpn_table_.stats(),
    };
  }

  void enable_all_optimizations(bool enabled) {
    ab_pruning_enabled_    = enabled;
    tpn_table_enabled_     = enabled;
    move_ordering_enabled_ = enabled;
  }

  void enable_ab_pruning(bool enabled) { ab_pruning_enabled_ = enabled; }
  void enable_tpn_table(bool enabled) { tpn_table_enabled_ = enabled; }
  void enable_move_ordering(bool enabled) { move_ordering_enabled_ = enabled; }

  void enable_tracing(std::ostream *os) {
    trace_os_     = os;
    trace_lineno_ = 0;
  }

  Game       &game() { return game_; }
  const Game &game() const { return game_; }

  Result solve();
  Result solve(int alpha, int beta);

private:
  int  solve_internal(int alpha, int beta, Cards &winners_by_rank);
  void order_plays(PlayOrder &order) const;

  void search_all_cards(
      int alpha, int beta, int &best_score, Cards &winners_by_rank
  );

  void trace(const char *tag, int alpha, int beta, int tricks_taken_by_ns);

  Game          game_;
  int64_t       nodes_explored_;
  TpnTable      tpn_table_;
  bool          ab_pruning_enabled_;
  bool          tpn_table_enabled_;
  bool          move_ordering_enabled_;
  std::ostream *trace_os_;
  int64_t       trace_lineno_;
};
