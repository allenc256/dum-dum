#pragma once

#include "Cards.h"

enum Seat : uint8_t {
  WEST,
  NORTH,
  EAST,
  SOUTH,
};

class State {
 public:
  State(Seat lead, Cards w, Cards n, Cards e, Cards s)
      : lead_(lead), w_(w), n_(n), e_(e), s_(s) {
    assert(w.disjoint(n) && w.disjoint(e) && w.disjoint(s));
    assert(n.disjoint(e) && n.disjoint(s));
    assert(e.disjoint(s));
  }

  Cards west() const { return w_; }
  Cards north() const { return n_; }
  Cards east() const { return e_; }
  Cards south() const { return s_; }

  static State random();

 private:
  Seat lead_;
  Cards w_, n_, e_, s_;
};

std::ostream& operator<<(std::ostream& os, Seat s);
std::ostream& operator<<(std::ostream& os, const State& s);
