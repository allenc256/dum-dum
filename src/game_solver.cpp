#include "game_solver.h"

#include <picosha2.h>

void Solver::State::init(const Game &g, int alpha, int beta, bool normalize) {
  assert(!g.current_trick().started());

  if (normalize) {
    Cards to_collapse;
    for (int i = 0; i < 4; i++) {
      to_collapse.add_all(g.hand((Seat)i));
    }
    to_collapse = to_collapse.complement();
    for (int i = 0; i < 4; i++) {
      hands[i] = g.hand((Seat)i).collapse_ranks(to_collapse);
    }
  } else {
    for (int i = 0; i < 4; i++) {
      hands[i] = g.hand((Seat)i);
    }
  }

  next_seat = g.next_seat();
  alpha     = (int8_t)(alpha - g.tricks_taken_by_ns());
  beta      = (int8_t)(beta - g.tricks_taken_by_ns());
}

Solver::Solver(Game g) : game_(g), states_explored_(0) {
  enable_all_optimizations(true);
}

Solver::~Solver() {}

Solver::Result Solver::solve() {
  states_explored_ = 0;
  Card best_play;
  int  tricks_taken_by_ns = solve_internal(0, game_.tricks_max(), &best_play);
  return Solver::Result(
      tricks_taken_by_ns,
      best_play,
      states_explored_,
      (int)transposition_table_.size()
  );
}

int Solver::solve_internal(int alpha, int beta, Card *best_play) {
  if (game_.finished()) {
    return game_.tricks_taken_by_ns();
  }

  bool  maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  State state;
  bool  state_init = false;

  if (!best_play && !game_.current_trick().started()) {
    state.init(game_, alpha, beta, state_normalization_enabled_);
    state_init = true;

    if (alpha_beta_pruning_enabled_) {
      int sure_tricks = count_sure_tricks(state);
      if (maximizing) {
        int worst_case = game_.tricks_taken_by_ns() + sure_tricks;
        if (worst_case >= beta) {
          return worst_case;
        }
      } else {
        int best_case =
            game_.tricks_taken_by_ns() + game_.tricks_left() - sure_tricks;
        if (best_case <= alpha) {
          return best_case;
        }
      }
    }

    if (transposition_table_enabled_) {
      auto it = transposition_table_.find(state);
      if (it != transposition_table_.end()) {
        return game_.tricks_taken_by_ns() + it->second;
      }
    }
  }

  int best_tricks_by_ns =
      solve_internal_search_plays(maximizing, alpha, beta, best_play);

  if (state_init && transposition_table_enabled_) {
    int tricks_takable_by_ns = best_tricks_by_ns - game_.tricks_taken_by_ns();
    assert(tricks_takable_by_ns >= 0);
    transposition_table_[state] = (uint8_t)tricks_takable_by_ns;
  }

  return best_tricks_by_ns;
}

int Solver::solve_internal_search_plays(
    bool maximizing, int &alpha, int &beta, Card *best_play
) {
  states_explored_++;

  int   best_tricks_by_ns = maximizing ? -1 : game_.tricks_max() + 1;
  Cards valid_plays       = game_.valid_plays().remove_equivalent_ranks();
  for (auto i = valid_plays.first(); i.valid(); i = valid_plays.next(i)) {
    if (solve_internal_search_single_play(
            i.card(), maximizing, alpha, beta, best_tricks_by_ns, best_play
        )) {
      break;
    }
  }
  return best_tricks_by_ns;
}

bool Solver::solve_internal_search_single_play(
    Card  c,
    bool  maximizing,
    int  &alpha,
    int  &beta,
    int  &best_tricks_by_ns,
    Card *best_play
) {
  game_.play(c);

  int  child_tricks_by_ns = solve_internal(alpha, beta, nullptr);
  bool prune              = false;
  if (maximizing) {
    if (child_tricks_by_ns > best_tricks_by_ns) {
      best_tricks_by_ns = child_tricks_by_ns;
      if (best_play) {
        *best_play = c;
      }
    }
    if (alpha_beta_pruning_enabled_) {
      if (best_tricks_by_ns >= beta) {
        prune = true;
      } else {
        alpha = std::max(alpha, best_tricks_by_ns);
      }
    }
  } else {
    if (child_tricks_by_ns < best_tricks_by_ns) {
      best_tricks_by_ns = child_tricks_by_ns;
      if (best_play) {
        *best_play = c;
      }
    }
    if (alpha_beta_pruning_enabled_) {
      if (best_tricks_by_ns <= alpha) {
        prune = true;
      } else {
        beta = std::min(beta, best_tricks_by_ns);
      }
    }
  }

  game_.unplay();
  return prune;
}

int Solver::count_sure_tricks(const State &state) const {
  if (game_.current_trick().started() || !state_normalization_enabled_) {
    return 0;
  }

  Seat next_seat = game_.next_seat();
  int  total     = 0;
  for (int suit_i = 0; suit_i < 4; suit_i++) {
    int count = state.hands[next_seat].top_ranks((Suit)suit_i);
    for (int i = 1; i < 4; i++) {
      Cards h = state.hands[right_seat(next_seat, i)];
      int   c = h.intersect_suit((Suit)suit_i).count();
      count   = std::min(count, c);
    }
    total += count;
  }

  return total;
}