#pragma once

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

struct Bounds {
  int8_t lower;
  int8_t upper;
};

typedef absl::flat_hash_map<GameState, Bounds> TranspositionTable;

class MiniSolver {
public:
  MiniSolver(Game &game, TranspositionTable &tp_table)
      : game_(game),
        tp_table_(tp_table) {}

  Bounds compute_bounds();

private:
  void play_my_lowest(Suit suit);
  void play_opp_lowest();
  void play_partner_lowest();
  void play_partner_ruff();

  Game               &game_;
  TranspositionTable &tp_table_;
};
