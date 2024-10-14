#pragma once

#include "game_model.h"
#include "tpn_table.h"

#include <absl/container/flat_hash_map.h>

class MiniSolver {
public:
  MiniSolver(Game &game, TpnTable &tpn_table)
      : game_(game),
        tpn_table_(tpn_table) {}

  TpnTableValue compute_value(int max_depth);

private:
  Card play_my_lowest(Suit suit);
  void play_opp_lowest();
  void play_partner_lowest();
  void play_partner_ruff();

  Game     &game_;
  TpnTable &tpn_table_;
};
