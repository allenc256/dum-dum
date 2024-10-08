
#include "abs_solver.h"

#include <iomanip>

std::ostream &operator<<(std::ostream &os, const PossTricks &t) {
  for (int i = 13; i >= 0; i--) {
    os << (t.contains(i) ? '1' : '0');
  }
  return os;
}

#define TRACE(tag, poss_tricks)                                                \
  {                                                                            \
    if (trace_os_) {                                                           \
      trace(tag, poss_tricks);                                                 \
    }                                                                          \
  }

AbsSolver::Result AbsSolver::solve() {
  states_explored_       = 0;
  PossTricks poss_tricks = solve_internal(-1, game_.tricks_max() + 1);
  return {.poss_tricks = poss_tricks, .states_explored = states_explored_};
}

PossTricks AbsSolver::solve_internal(int alpha, int beta) {
  if (game_.finished()) {
    auto poss_tricks = PossTricks::only(game_.tricks_taken_by_ns());
    TRACE("terminal", &poss_tricks);
    return poss_tricks;
  }

  states_explored_++;

  TRACE("start", nullptr);

  bool       maximizing  = is_maximizing();
  AbsCards   valid_plays = game_.valid_plays();
  PossTricks poss_tricks;
  for (auto it = valid_plays.iter(); it.valid(); it.next()) {
    game_.play(it.card());
    PossTricks child_tricks = solve_internal_child(alpha, beta);
    game_.unplay();
    if (maximizing) {
      poss_tricks = poss_tricks.max(child_tricks);
      int lb      = poss_tricks.lower_bound();
      alpha       = std::max(alpha, lb);
      if (lb >= beta) {
        break;
      }
    } else {
      poss_tricks = poss_tricks.min(child_tricks);
      int ub      = poss_tricks.upper_bound();
      beta        = std::min(beta, ub);
      if (ub <= alpha) {
        break;
      }
    }
  }

  TRACE("end", &poss_tricks);

  return poss_tricks;
}

PossTricks AbsSolver::solve_internal_child(int alpha, int beta) {
  if (game_.trick_state() == AbsTrick::FINISHING) {
    PossTricks result;
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      if (game_.can_finish_trick(seat)) {
        game_.finish_trick(seat);
        result.add_all(solve_internal(alpha, beta));
        game_.unfinish_trick();
      }
    }
    return result;
  } else {
    return solve_internal(alpha, beta);
  }
}

void AbsSolver::trace(const char *tag, const PossTricks *poss_tricks) {
  *trace_os_ << std::left << std::setw(8) << trace_lineno_ << ' ' << std::right;
  *trace_os_ << (is_maximizing() ? "max" : "min") << ' ';
  *trace_os_ << std::setw(8) << tag << ' ';
  if (poss_tricks) {
    *trace_os_ << *poss_tricks << std::setw(3) << poss_tricks->lower_bound()
               << std::setw(3) << poss_tricks->upper_bound();
  } else {
    for (int i = 0; i < 20; i++) {
      *trace_os_ << ' ';
    }
  }
  for (int i = 0;
       i < game_.tricks_max() && game_.trick(i).state() != AbsTrick::STARTING;
       i++) {
    *trace_os_ << ' ' << game_.trick(i);
  }
  *trace_os_ << std::endl;
  trace_lineno_++;
}
