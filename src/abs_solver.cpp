
#include "abs_solver.h"

#include <iomanip>

#define TRACE(tag, bounds)                                                     \
  {                                                                            \
    if (trace_os_) {                                                           \
      trace(tag, bounds);                                                      \
    }                                                                          \
  }

AbsSolver::Result AbsSolver::solve() {
  states_explored_ = 0;
  AbsBounds bounds = solve_internal(-1, game_.tricks_max() + 1);
  return {
      .bounds          = bounds,
      .states_explored = states_explored_,
      .states_memoized = (int64_t)tpn_table_.size(),
  };
}

AbsBounds AbsSolver::solve_internal(int alpha, int beta) {
  if (game_.finished()) {
    AbsBounds bounds =
        AbsBounds(game_.tricks_taken_by_ns(), game_.tricks_taken_by_ns());
    TRACE("terminal", &bounds);
    return bounds;
  }

  AbsState game_state;
  if (game_.trick_state() == AbsTrick::STARTING) {
    game_state.init(game_);
    AbsBounds bounds = tpn_table_.lookup(game_, game_state);
    if (bounds.lower() >= beta) {
      return bounds;
    }
    if (bounds.upper() <= alpha) {
      return bounds;
    }
  }

  states_explored_++;

  bool      maximizing  = is_maximizing();
  AbsCards  valid_plays = game_.valid_plays();
  AbsBounds bounds;

  TRACE("start", nullptr);

  for (auto it = valid_plays.iter(); it.valid(); it.next()) {
    game_.play(it.card());
    AbsBounds child_bounds = solve_internal_child(alpha, beta);
    game_.unplay();
    if (maximizing) {
      bounds = bounds.max(child_bounds);
      alpha  = std::max(alpha, bounds.lower());
      if (bounds.lower() >= beta) {
        break;
      }
    } else {
      bounds = bounds.min(child_bounds);
      beta   = std::min(beta, bounds.upper());
      if (bounds.upper() <= alpha) {
        break;
      }
    }
  }

  TRACE("end", &bounds);

  if (game_.trick_state() == AbsTrick::STARTING) {
    tpn_table_.update(game_, game_state, bounds);
  }

  return bounds;
}

AbsBounds AbsSolver::solve_internal_child(int alpha, int beta) {
  if (game_.trick_state() == AbsTrick::FINISHING) {
    // int poss_winners = 0;
    // for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    //   if (game_.can_finish_trick(seat)) {
    //     poss_winners++;
    //   }
    // }

    AbsBounds bounds;
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      if (game_.can_finish_trick(seat)) {
        game_.finish_trick(seat);
        bounds.extend(solve_internal(alpha, beta));
        // if (poss_winners == 1) {
        //   bounds.extend(solve_internal(alpha, beta));
        // } else {
        //   AbsBounds trivial_bounds = AbsBounds(
        //       game_.tricks_taken_by_ns(),
        //       game_.tricks_taken_by_ns() + game_.tricks_left()
        //   );
        //   bounds.extend(trivial_bounds);
        // }
        game_.unfinish_trick();
      }
    }
    return bounds;
  } else {
    return solve_internal(alpha, beta);
  }
}

void AbsSolver::trace(const char *tag, const AbsBounds *bounds) {
  *trace_os_ << std::left << std::setw(8) << trace_lineno_ << ' ' << std::right;
  *trace_os_ << (is_maximizing() ? "max" : "min") << ' ';
  *trace_os_ << std::setw(8) << tag << ' ';
  if (bounds) {
    *trace_os_ << std::setw(3) << bounds->lower() << std::setw(3)
               << bounds->upper();
  } else {
    for (int i = 0; i < 6; i++) {
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
