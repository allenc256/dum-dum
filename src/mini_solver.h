#pragma once

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

struct Bounds {
  int8_t lower;
  int8_t upper;
  int8_t max_depth;
  Card   best_play;
};

typedef absl::flat_hash_map<GameState, Bounds> TpnTable;

class MiniSolver {
public:
  MiniSolver(Game &game, TpnTable &tpn_table)
      : game_(game),
        tpn_table_(tpn_table) {}

  Bounds compute_bounds(int max_depth);

private:
  Card play_my_lowest(Suit suit);
  void play_opp_lowest();
  void play_partner_lowest();
  void play_partner_ruff();

  Game     &game_;
  TpnTable &tpn_table_;
};
