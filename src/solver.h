#pragma once

#include "game_model.h"
#include "tpn_table.h"

#include <absl/container/flat_hash_map.h>
#include <array>
#include <ostream>
#include <vector>

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

  Stats stats() const;

  void enable_all_optimizations(bool enabled);
  void enable_ab_pruning(bool enabled);
  void enable_tpn_table(bool enabled);
  void enable_play_order(bool enabled);
  void enable_fast_tricks(bool enabled);
  void enable_tracing(std::ostream *os);

  Game       &game() { return game_; }
  const Game &game() const { return game_; }

  Result solve();
  Result solve(int alpha, int beta);

private:
  int  solve_internal(int alpha, int beta, Cards &winners_by_rank);
  bool prune_fast_tricks(
      int alpha, int beta, int &score, Cards &winners_by_rank
  ) const;
  void search_all_cards(
      int alpha, int beta, int &best_score, Cards &winners_by_rank
  );
  void trace(const char *tag, int alpha, int beta, int tricks_taken_by_ns);

  Game          game_;
  int64_t       nodes_explored_;
  TpnTable      tpn_table_;
  bool          ab_pruning_enabled_;
  bool          tpn_table_enabled_;
  bool          play_order_enabled_;
  bool          fast_tricks_enabled_;
  std::ostream *trace_os_;
  int64_t       trace_lineno_;
};
