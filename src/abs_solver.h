#pragma once

#include "abs_game_model.h"

#include <bit>

class AbsBounds {
public:
  AbsBounds() : lower_(13), upper_(0) {}

  AbsBounds(int lower, int upper)
      : lower_((int8_t)lower),
        upper_((int8_t)upper) {
    assert(lower >= 0 && lower <= 13);
    assert(upper >= 0 && upper <= 13);
  }

  bool empty() const { return lower_ > upper_; }
  int  lower() const { return lower_; }
  int  upper() const { return upper_; }
  bool contains(int n) const { return n >= lower_ && n <= upper_; }

  void extend(AbsBounds other) {
    lower_ = std::min(lower_, other.lower_);
    upper_ = std::max(upper_, other.upper_);
  }

  AbsBounds max(AbsBounds other) {
    if (empty()) {
      return other;
    } else if (other.empty()) {
      return *this;
    } else {
      int lower = std::max(lower_, other.lower_);
      int upper = std::max(upper_, other.upper_);
      return AbsBounds(lower, upper);
    }
  }

  AbsBounds min(AbsBounds other) {
    if (empty()) {
      return other;
    } else if (other.empty()) {
      return *this;
    } else {
      int lower = std::min(lower_, other.lower_);
      int upper = std::min(upper_, other.upper_);
      return AbsBounds(lower, upper);
    }
  }

  friend bool operator==(const AbsBounds &p1, const AbsBounds &p2) {
    return p1.lower_ == p2.lower_ && p1.upper_ == p2.upper_;
  }

  friend std::ostream &operator<<(std::ostream &os, const AbsBounds &b) {
    return os << '[' << (int)b.lower_ << ", " << (int)b.upper_ << ']';
  }

private:
  int8_t lower_;
  int8_t upper_;
};

class AbsSolver {
public:
  AbsSolver(AbsGame game)
      : game_(game),
        states_explored_(0),
        trace_os_(nullptr),
        trace_lineno_(0) {}

  struct Result {
    AbsBounds bounds;
    int64_t   states_explored;
  };

  Result solve();

  void enable_tracing(std::ostream *os) {
    trace_os_     = os;
    trace_lineno_ = 0;
  }

private:
  bool is_maximizing() const {
    return game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  }

  AbsBounds solve_internal(int alpha, int beta);
  AbsBounds solve_internal_child(int alpha, int beta);

  void trace(const char *tag, const AbsBounds *bounds);

  AbsGame       game_;
  int64_t       states_explored_;
  std::ostream *trace_os_;
  int64_t       trace_lineno_;
};
