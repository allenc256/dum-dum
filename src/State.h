#pragma once

#include <random>

#include "Cards.h"

enum Seat : uint8_t {
  WEST,
  NORTH,
  EAST,
  SOUTH,
};

class State {
public:
  State(Seat lead, Cards cards[4]) : lead_(lead) {
    for (int i = 0; i < 4; i++) {
      cards_[i] = cards[i];
    }
  }

  Cards cards(Seat seat) const { return cards_[seat]; }

  static State random(std::default_random_engine &random);

private:
  __unused Seat lead_;
  Cards cards_[4];
};

std::ostream &operator<<(std::ostream &os, Seat s);
std::ostream &operator<<(std::ostream &os, const State &s);
