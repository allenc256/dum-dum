#pragma once

#include "abs_game_model.h"

#include <bit>

class PossTricks {
public:
  PossTricks() : bits_(0) {}

  bool empty() const { return bits_ == 0; }
  bool contains(int n) const { return bits_ & (1 << n); }

  int lower_bound() const {
    if (empty()) {
      return 0;
    } else {
      return std::countr_zero(bits_);
    }
  }

  int upper_bound() const {
    if (empty()) {
      return 13;
    } else {
      return 15 - std::countl_zero(bits_);
    }
  }

  void add_all(PossTricks other) { bits_ |= other.bits_; }

  PossTricks union_with(PossTricks other) const {
    return PossTricks(bits_ | other.bits_);
  }

  PossTricks intersect(PossTricks other) const {
    return PossTricks(bits_ & other.bits_);
  }

  PossTricks max(PossTricks other) const {
    int lb = std::max(lower_bound(), other.lower_bound());
    return union_with(other).intersect(at_least(lb));
  }

  PossTricks min(PossTricks other) const {
    int ub = std::min(upper_bound(), other.upper_bound());
    return union_with(other).intersect(at_most(ub));
  }

  static PossTricks at_least(int n) {
    assert(n >= 0 && n <= 13);
    return PossTricks((ALL_MASK << n) & ALL_MASK);
  }

  static PossTricks at_most(int n) {
    assert(n >= 0 && n <= 13);
    return PossTricks(ALL_MASK >> (13 - n));
  }

  static PossTricks between(int lb, int ub) {
    return at_least(lb).intersect(at_most(ub));
  }

  static PossTricks only(int n) {
    assert(n >= 0 && n <= 13);
    return PossTricks((uint16_t)(1 << n));
  }

  friend bool operator==(const PossTricks &p1, const PossTricks &p2) {
    return p1.bits_ == p2.bits_;
  }

private:
  PossTricks(uint16_t bits) : bits_(bits) { assert(!(bits & ~ALL_MASK)); }

  static constexpr uint16_t ALL_MASK = 0b11111111111111;

  uint16_t bits_;
};

std::ostream &operator<<(std::ostream &os, const PossTricks &t);

class AbsSolver {
public:
  AbsSolver(AbsGame game)
      : game_(game),
        states_explored_(0),
        trace_os_(nullptr),
        trace_lineno_(0) {}

  struct Result {
    PossTricks poss_tricks;
    int64_t    states_explored;
  };

  Result solve();

  void enable_tracing(std::ostream *os) {
    trace_os_     = os;
    trace_lineno_ = 0;
  }

private:
  PossTricks solve_internal();

  bool is_maximizing() const {
    return game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  }

  PossTricks solve_internal_child();

  void trace(const char *tag, const PossTricks *poss_tricks);

  AbsGame       game_;
  int64_t       states_explored_;
  std::ostream *trace_os_;
  int64_t       trace_lineno_;
};
