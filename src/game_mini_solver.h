#pragma once

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

class MiniSolver {
public:
  struct GameState {
    std::array<Cards, 4> hands;
    Seat                 next_seat;

    GameState(const Game &game);

    template <typename H> friend H AbslHashValue(H h, const GameState &s) {
      return H::combine(std::move(h), s.hands, s.next_seat);
    }

    friend bool operator==(const GameState &s1, const GameState &s2) {
      return s1.hands == s2.hands && s1.next_seat == s2.next_seat;
    }
  };

  MiniSolver(Game &game) : game_(game) {}

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
