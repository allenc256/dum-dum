#pragma once

#include "game_model.h"
#include "tpn_table.h"

class MiniSolver {
public:
  MiniSolver(Game &game, TpnTable &tpn_table)
      : game_(game),
        tpn_table_(tpn_table) {}

  void solve(int max_depth, TpnTable::Value &value);

private:
  void solve_child(int max_depth, Card my_play, TpnTable::Value &value);

  Card play_my_lowest(Suit suit);
  void play_opp_lowest();
  void play_partner_lowest();
  void play_partner_ruff();

  Game     &game_;
  TpnTable &tpn_table_;
};
