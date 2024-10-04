#pragma once

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

class MiniSolver {
public:
  MiniSolver(Game &game) : game_(game) {}

  int64_t states_memoized() const { return tp_table_.size(); }

  int count_forced_tricks();

private:
  typedef absl::flat_hash_map<GameState, uint8_t> TranspositionTable;

  void play_my_lowest(Suit suit);
  void play_opp_lowest();
  void play_partner_lowest();
  void play_partner_ruff();

  Game              &game_;
  TranspositionTable tp_table_;
};
