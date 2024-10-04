#include "solver.h"

#include <picosha2.h>

Solver::Solver(Game g)
    : game_(g),
      states_explored_(0),
      mini_solver_(game_),
      trace_ostream_(nullptr),
      trace_lineno_(0) {
  enable_all_optimizations(true);
}

Solver::~Solver() {}

Solver::Result Solver::solve() { return solve(-1, game_.tricks_max() + 1); }

Solver::Result Solver::solve(int alpha, int beta) {
  states_explored_ = 0;
  Card best_play;
  int  tricks_taken_by_ns = solve_internal(alpha, beta, &best_play);
  int  tricks_taken_by_ew = game_.tricks_max() - tricks_taken_by_ns;
  return {
      .tricks_taken_by_ns = tricks_taken_by_ns,
      .tricks_taken_by_ew = tricks_taken_by_ew,
      .best_play          = best_play,
      .states_explored    = states_explored_,
      .states_memoized    = (int64_t)tp_table_.size(),
      .ms_states_memoized = mini_solver_.states_memoized(),
  };
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

  bool   maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  Cards  ignorable  = game_.ignorable_cards();
  Bounds table_bounds;
  GameState game_state;

  if (game_.start_of_trick()) {
    game_state.init(game_, ignorable);

    if (!best_play) {
      if (mini_solver_enabled_) {
        int result = search_forced_tricks(maximizing, alpha, beta);
        if (result >= 0) {
          TRACE("prune", &game_state, alpha, beta, result);
          return result;
        }
      }

      if (tp_table_enabled_) {
        auto it = tp_table_.find(game_state);
        if (it != tp_table_.end()) {
          table_bounds = it->second;
        } else {
          table_bounds = {.lower = 0, .upper = (int8_t)game_.tricks_left()};
        }
        int lower = table_bounds.lower + game_.tricks_taken_by_ns();
        int upper = table_bounds.upper + game_.tricks_taken_by_ns();
        if (lower >= beta) {
          TRACE("lookup", &game_state, alpha, beta, lower);
          return lower;
        }
        if (upper <= alpha) {
          TRACE("lookup", &game_state, alpha, beta, upper);
          return upper;
        }
      }
    }

    TRACE("start", &game_state, alpha, beta, -1);
  }

  SearchState search_state = {
      .ignorable         = ignorable,
      .maximizing        = maximizing,
      .alpha             = alpha,
      .beta              = beta,
      .best_tricks_by_ns = maximizing ? -1 : game_.tricks_max() + 1,
      .best_play         = best_play,
  };
  search_all_cards(search_state);

  if (game_.start_of_trick()) {
    TRACE("end", &game_state, alpha, beta, search_state.best_tricks_by_ns);

    if (tp_table_enabled_) {
      int8_t best    = (int8_t)search_state.best_tricks_by_ns;
      int8_t taken   = (int8_t)game_.tricks_taken_by_ns();
      bool   changed = false;
      if (best < beta) {
        table_bounds.upper = best - taken;
        changed            = true;
      }
      if (best > alpha) {
        table_bounds.lower = best - taken;
        changed            = true;
      }
      if (changed) {
        tp_table_[game_state] = table_bounds;
      }
    }
  }

  return search_state.best_tricks_by_ns;
}

bool Solver::search_all_cards(SearchState &s) {
  states_explored_++;

  const Trick &t        = game_.current_trick();
  Cards        my_plays = game_.valid_plays();

  if (move_ordering_enabled_) {
    Cards my_winners = my_plays.intersect(t.winning_cards());
    Cards my_losers  = my_plays.subtract(my_winners);

    if (t.card_count() == 1) {
      Cards partner_plays   = t.valid_plays(game_.hand(t.seat(3)));
      Cards partner_winners = partner_plays.intersect(t.winning_cards());
      if (partner_winners.empty()) {
        if (search_specific_cards(s, my_winners, HIGH_TO_LOW)) {
          return true;
        }
      } else {
        if (search_specific_cards(s, my_losers, LOW_TO_HIGH)) {
          return true;
        }
      }
    } else if (t.card_count() == 2) {
      Cards opp_plays   = t.valid_plays(game_.hand(t.seat(3)));
      Cards opp_winners = opp_plays.intersect(t.winning_cards());
      if (opp_winners.empty()) {
        if (search_specific_cards(s, my_losers, LOW_TO_HIGH)) {
          return true;
        }
      } else {
        if (search_specific_cards(s, my_winners, HIGH_TO_LOW)) {
          return true;
        }
      }
    } else if (t.card_count() == 3) {
      bool partner_winning = t.winning_index() == 1;
      if (partner_winning) {
        if (search_specific_cards(s, my_losers, LOW_TO_HIGH)) {
          return true;
        }
      } else {
        if (search_specific_cards(s, my_winners, LOW_TO_HIGH)) {
          return true;
        }
      }
    }
  }

  return search_specific_cards(s, my_plays, LOW_TO_HIGH);
}

bool Solver::search_specific_cards(SearchState &s, Cards c, Order o) {
  c = c.subtract(s.already_searched);
  if (c.empty()) {
    return false;
  }
  s.already_searched.add_all(c);
  c = c.prune_equivalent(s.ignorable);
  if (o == HIGH_TO_LOW) {
    for (auto i = c.iter_highest(); i.valid(); i = c.iter_lower(i)) {
      if (search_specific_card(s, i.card())) {
        return true;
      }
    }
  } else {
    for (auto i = c.iter_lowest(); i.valid(); i = c.iter_higher(i)) {
      if (search_specific_card(s, i.card())) {
        return true;
      }
    }
  }
  return false;
}

bool Solver::search_specific_card(SearchState &s, Card c) {
  game_.play(c);

  int  child_tricks_by_ns = solve_internal(s.alpha, s.beta, nullptr);
  bool prune              = false;
  if (s.maximizing) {
    if (child_tricks_by_ns > s.best_tricks_by_ns) {
      s.best_tricks_by_ns = child_tricks_by_ns;
      if (s.best_play) {
        *s.best_play = c;
      }
    }
    if (ab_pruning_enabled_) {
      s.alpha = std::max(s.alpha, s.best_tricks_by_ns);
      if (s.best_tricks_by_ns >= s.beta) {
        prune = true;
      }
    }
  } else {
    if (child_tricks_by_ns < s.best_tricks_by_ns) {
      s.best_tricks_by_ns = child_tricks_by_ns;
      if (s.best_play) {
        *s.best_play = c;
      }
    }
    if (ab_pruning_enabled_) {
      s.beta = std::min(s.beta, s.best_tricks_by_ns);
      if (s.best_tricks_by_ns <= s.alpha) {
        prune = true;
      }
    }
  }

  game_.unplay();
  return prune;
}

int Solver::search_forced_tricks(bool maximizing, int &alpha, int &beta) {
  assert(mini_solver_enabled_);
  int forced_tricks = mini_solver_.count_forced_tricks();
  if (maximizing) {
    int worst_case = game_.tricks_taken_by_ns() + forced_tricks;
    if (worst_case >= beta) {
      return worst_case;
    }
    alpha = std::max(alpha, worst_case);
  } else {
    int best_case =
        game_.tricks_taken_by_ns() + game_.tricks_left() - forced_tricks;
    if (best_case <= alpha) {
      return best_case;
    }
    beta = std::min(beta, best_case);
  }
  return -1;
}

static void sha256_hash(const GameState *state, char *buf, size_t buflen) {
  if (!state) {
    buf[0] = 0;
    return;
  }
  uint8_t  digest[32];
  uint8_t *in_begin = (uint8_t *)state;
  uint8_t *in_end   = in_begin + sizeof(GameState);
  picosha2::hash256(in_begin, in_end, digest, digest + 32);
  std::snprintf(
      buf, buflen, "%08x%08x", *(uint32_t *)&digest[0], *(uint32_t *)&digest[4]
  );
}

void Solver::trace(
    const char      *tag,
    const GameState *state,
    int              alpha,
    int              beta,
    int              tricks_taken_by_ns
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
      "%-7llu %-8s %16s %2d %2d %2d %s",
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
