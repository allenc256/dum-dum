#include "game_solver.h"

#include <picosha2.h>

void Solver::State::init(
    const Game &g, int alpha_param, int beta_param, bool normalize
) {
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
  alpha     = (uint8_t)std::max(0, alpha_param - g.tricks_taken_by_ns());
  beta      = (uint8_t)std::max(0, beta_param - g.tricks_taken_by_ns());
}

Solver::Solver(Game g)
    : game_(g),
      states_explored_(0),
      trace_ostream_(nullptr),
      trace_lineno_(0) {
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

#define TRACE(tag, state, alpha, beta, tricks_taken_by_ns)                     \
  if (trace_ostream_) {                                                        \
    trace(tag, state, alpha, beta, tricks_taken_by_ns);                        \
  }

int Solver::solve_internal(int alpha, int beta, Card *best_play) {
  if (game_.finished()) {
    TRACE("terminal", nullptr, alpha, beta, game_.tricks_taken_by_ns());
    return game_.tricks_taken_by_ns();
  }

  bool  maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  bool  start_of_trick = !game_.current_trick().started();
  State state;

  if (start_of_trick) {
    state.init(game_, alpha, beta, state_normalization_enabled_);

    if (!best_play) {
      if (alpha_beta_pruning_enabled_) {
        int sure_tricks = count_sure_tricks(state);
        if (maximizing) {
          int worst_case = game_.tricks_taken_by_ns() + sure_tricks;
          if (worst_case >= beta) {
            TRACE("prune", &state, alpha, beta, worst_case);
            return worst_case;
          }
        } else {
          int best_case =
              game_.tricks_taken_by_ns() + game_.tricks_left() - sure_tricks;
          if (best_case <= alpha) {
            TRACE("prune", &state, alpha, beta, best_case);
            return best_case;
          }
        }
      }

      if (transposition_table_enabled_) {
        auto it = transposition_table_.find(state);
        if (it != transposition_table_.end()) {
          int tricks_taken_by_ns = game_.tricks_taken_by_ns() + it->second;
          TRACE("lookup", &state, alpha, beta, tricks_taken_by_ns);
          return tricks_taken_by_ns;
        }
      }
    }

    TRACE("start", &state, alpha, beta, -1);
  }

  int best_tricks_by_ns =
      solve_internal_search_plays(maximizing, alpha, beta, best_play);

  if (start_of_trick) {
    TRACE("end", &state, alpha, beta, best_tricks_by_ns);

    if (transposition_table_enabled_) {
      int tricks_takable_by_ns = best_tricks_by_ns - game_.tricks_taken_by_ns();
      assert(tricks_takable_by_ns >= 0);
      transposition_table_[state] = (uint8_t)tricks_takable_by_ns;
    }
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

static void sha256_hash(const Solver::State *state, char *buf, size_t buflen) {
  if (!state) {
    buf[0] = 0;
    return;
  }
  uint8_t  digest[32];
  uint8_t *in_begin = (uint8_t *)state;
  uint8_t *in_end   = in_begin + sizeof(Solver::State);
  picosha2::hash256(in_begin, in_end, digest, digest + 32);
  std::snprintf(
      buf, buflen, "%08x%08x", *(uint32_t *)&digest[0], *(uint32_t *)&digest[4]
  );
}

void Solver::trace(
    const char  *tag,
    const State *state,
    int          alpha,
    int          beta,
    int          tricks_taken_by_ns
) {
  char line_buf[121], tricks_buf[3], hash_buf[17];
  sha256_hash(state, hash_buf, sizeof(hash_buf));
  if (tricks_taken_by_ns >= 0) {
    std::snprintf(tricks_buf, sizeof(tricks_buf), "%2d", tricks_taken_by_ns);
  } else {
    std::strcpy(tricks_buf, "  ");
  }
  std::snprintf(
      line_buf,
      sizeof(line_buf),
      "%-7d %-8s %16s %2d %2d %2d %s",
      trace_lineno_,
      tag,
      hash_buf,
      alpha,
      beta,
      game_.tricks_taken_by_ns(),
      tricks_buf
  );
  *trace_ostream_ << line_buf;
  for (int i = 0; i < game_.tricks_taken(); i++) {
    *trace_ostream_ << " " << game_.trick(i);
  }
  *trace_ostream_ << std::endl;
  trace_lineno_++;
}
