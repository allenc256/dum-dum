#include "game_solver.h"

#include <picosha2.h>

void Solver::GameState::init(
    const Game &g, int alpha_param, int beta_param, bool normalize
) {
  assert(!g.current_trick().started());

  if (normalize) {
    Cards to_collapse;
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      to_collapse.add_all(g.hand(seat));
    }
    to_collapse = to_collapse.complement();
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      hands[seat] = g.hand(seat).collapse_ranks(to_collapse);
    }
  } else {
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      hands[seat] = g.hand(seat);
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
      tricks_taken_by_ns, best_play, states_explored_, (int)tp_table_.size()
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

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  bool start_of_trick = !game_.current_trick().started();
  GameState game_state;

  if (start_of_trick) {
    game_state.init(game_, alpha, beta, tp_table_norm_enabled_);

    if (!best_play) {
      if (sure_tricks_enabled_) {
        int result = search_sure_tricks(game_state, maximizing, alpha, beta);
        if (result >= 0) {
          return result;
        }
      }

      if (tp_table_enabled_) {
        auto it = tp_table_.find(game_state);
        if (it != tp_table_.end()) {
          int tricks_taken_by_ns = game_.tricks_taken_by_ns() + it->second;
          TRACE("lookup", &game_state, alpha, beta, tricks_taken_by_ns);
          return tricks_taken_by_ns;
        }
      }
    }

    TRACE("start", &game_state, alpha, beta, -1);
  }

  SearchState search_state = {
      .maximizing        = maximizing,
      .alpha             = alpha,
      .beta              = beta,
      .best_tricks_by_ns = maximizing ? -1 : game_.tricks_max() + 1,
      .best_play         = best_play,
  };
  search_all_cards(search_state);

  if (start_of_trick) {
    TRACE("end", &game_state, alpha, beta, search_state.best_tricks_by_ns);

    if (tp_table_enabled_) {
      int tricks_takable_by_ns =
          search_state.best_tricks_by_ns - game_.tricks_taken_by_ns();
      assert(tricks_takable_by_ns >= 0);
      tp_table_[game_state] = (uint8_t)tricks_takable_by_ns;
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
  c = c.remove_equivalent_ranks();
  if (o == HIGH_TO_LOW) {
    for (auto i = c.iter_high(); i.valid(); i = c.iter_lower(i)) {
      if (search_specific_card(s, i.card())) {
        return true;
      }
    }
  } else {
    for (auto i = c.iter_low(); i.valid(); i = c.iter_higher(i)) {
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
      if (s.best_tricks_by_ns >= s.beta) {
        prune = true;
      } else {
        s.alpha = std::max(s.alpha, s.best_tricks_by_ns);
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
      if (s.best_tricks_by_ns <= s.alpha) {
        prune = true;
      } else {
        s.beta = std::min(s.beta, s.best_tricks_by_ns);
      }
    }
  }

  game_.unplay();
  return prune;
}

int Solver::search_sure_tricks(
    const GameState &state, bool maximizing, int &alpha, int &beta
) {
  assert(sure_tricks_enabled_);
  int sure_tricks = count_sure_tricks(state);
  if (maximizing) {
    int worst_case = game_.tricks_taken_by_ns() + sure_tricks;
    if (worst_case >= beta) {
      TRACE("prune", &state, alpha, beta, worst_case);
      return worst_case;
    }
    alpha = std::max(alpha, worst_case);
  } else {
    int best_case =
        game_.tricks_taken_by_ns() + game_.tricks_left() - sure_tricks;
    if (best_case <= alpha) {
      TRACE("prune", &state, alpha, beta, best_case);
      return best_case;
    }
    beta = std::min(beta, best_case);
  }
  return -1;
}

int Solver::count_sure_tricks(const GameState &state) const {
  if (game_.current_trick().started() || !tp_table_norm_enabled_) {
    return 0;
  }

  Seat next_seat = game_.next_seat();
  int  total     = 0;
  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    int count = state.hands[next_seat].top_ranks(suit);
    for (int i = 1; i < 4; i++) {
      Cards h = state.hands[right_seat(next_seat, i)];
      int   c = h.intersect_suit(suit).count();
      count   = std::min(count, c);
    }
    total += count;
  }

  return total;
}

static void
sha256_hash(const Solver::GameState *state, char *buf, size_t buflen) {
  if (!state) {
    buf[0] = 0;
    return;
  }
  uint8_t  digest[32];
  uint8_t *in_begin = (uint8_t *)state;
  uint8_t *in_end   = in_begin + sizeof(Solver::GameState);
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
