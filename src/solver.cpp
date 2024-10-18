#include "solver.h"

#include <picosha2.h>

Solver::Solver(Game g)
    : game_(g),
      search_ply_(0),
      states_explored_(0),
      tpn_table_(game_),
      mini_solver_(game_, tpn_table_),
      trace_ostream_(nullptr),
      trace_lineno_(0) {
  enable_all_optimizations(true);
}

Solver::~Solver() {}

Solver::Result Solver::solve() {
  return solve(-1, game_.tricks_max() + 1, game_.tricks_max());
}

Solver::Result Solver::solve(int alpha, int beta, int max_depth) {
  states_explored_ = 0;
  search_ply_      = 0;
  best_play_.reset();
  int tricks_taken_by_ns = solve_internal(alpha, beta, max_depth);
  int tricks_taken_by_ew = game_.tricks_max() - tricks_taken_by_ns;
  assert(best_play_.has_value());
  return {
      .tricks_taken_by_ns = tricks_taken_by_ns,
      .tricks_taken_by_ew = tricks_taken_by_ew,
      .best_play          = *best_play_,
      .states_explored    = states_explored_,
      .states_memoized    = (int64_t)tpn_table_.size(),
  };
}

#define TRACE(tag, alpha, beta, tricks_taken_by_ns)                            \
  if (trace_ostream_) {                                                        \
    trace(tag, alpha, beta, tricks_taken_by_ns);                               \
  }

int Solver::solve_internal(int alpha, int beta, int max_depth) {
  if (game_.finished()) {
    TRACE("terminal", alpha, beta, game_.tricks_taken_by_ns());
    return game_.tricks_taken_by_ns();
  }

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  TpnTableValue tt_value;

  if (game_.start_of_trick()) {
    if (tpn_table_enabled_) {
      tt_value = compute_initial_value(max_depth);
      if (tt_value.lower_bound() >= beta || tt_value.has_tight_bounds()) {
        TRACE("prune", alpha, beta, tt_value.lower_bound());
        if (search_ply_ == 0) {
          best_play_ = tt_value.pv_play();
        }
        return tt_value.lower_bound();
      }
      if (tt_value.upper_bound() <= alpha) {
        TRACE("prune", alpha, beta, tt_value.upper_bound());
        if (search_ply_ == 0) {
          best_play_ = tt_value.pv_play();
        }
        return tt_value.upper_bound();
      }
      assert(!tt_value.has_tight_bounds());
    }

    TRACE("start", alpha, beta, -1);
  }

  SearchState search_state = {
      .maximizing        = maximizing,
      .alpha             = alpha,
      .beta              = beta,
      .best_tricks_by_ns = maximizing ? -1 : game_.tricks_max() + 1,
      .max_depth         = max_depth
  };
  search_all_cards(search_state);

  if (game_.start_of_trick()) {
    TRACE("end", alpha, beta, search_state.best_tricks_by_ns);

    if (tpn_table_enabled_) {
      bool changed = false;
      if (search_state.best_tricks_by_ns < beta) {
        if (tt_value.update_upper_bound(search_state.best_tricks_by_ns)) {
          changed = true;
        }
      }
      if (search_state.best_tricks_by_ns > alpha) {
        if (tt_value.update_lower_bound(search_state.best_tricks_by_ns)) {
          changed = true;
        }
      }
      if (changed) {
        if (tt_value.has_tight_bounds()) {
          tt_value.update_pv_play(search_state.best_play);
        }
        tpn_table_.update_value(tt_value);
      }
    }
  }

  bool is_pv_node = alpha < search_state.best_tricks_by_ns &&
                    search_state.best_tricks_by_ns < beta;
  if (search_ply_ == 0 && is_pv_node) {
    best_play_ = search_state.best_play;
  }

  return search_state.best_tricks_by_ns;
}

TpnTableValue Solver::compute_initial_value(int max_depth) {
  if (mini_solver_enabled_) {
    return mini_solver_.compute_value(max_depth);
  } else {
    return tpn_table_.lookup_value(max_depth);
  }
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
  c = game_.prune_equivalent_cards(c);
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
  search_ply_++;

  int  child_tricks_by_ns = solve_internal(s.alpha, s.beta, s.max_depth);
  bool prune              = false;
  if (s.maximizing) {
    if (child_tricks_by_ns > s.best_tricks_by_ns) {
      s.best_tricks_by_ns = child_tricks_by_ns;
      s.best_play         = c;
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
      s.best_play         = c;
    }
    if (ab_pruning_enabled_) {
      s.beta = std::min(s.beta, s.best_tricks_by_ns);
      if (s.best_tricks_by_ns <= s.alpha) {
        prune = true;
      }
    }
  }

  search_ply_--;
  game_.unplay();
  return prune;
}

static void sha256_hash(Game &game, char *buf, size_t buflen) {
  if (!game.start_of_trick()) {
    buf[0] = 0;
    return;
  }
  const GameKey &key = game.normalized_key();
  uint8_t        digest[32];
  uint8_t       *in_begin = (uint8_t *)&key;
  uint8_t       *in_end   = in_begin + sizeof(GameKey);
  picosha2::hash256(in_begin, in_end, digest, digest + 32);
  std::snprintf(
      buf, buflen, "%08x%08x", *(uint32_t *)&digest[0], *(uint32_t *)&digest[4]
  );
}

void Solver::trace(
    const char *tag, int alpha, int beta, int tricks_taken_by_ns
) {
  char line_buf[121], tricks_buf[3], hash_buf[17];
  sha256_hash(game_, hash_buf, sizeof(hash_buf));
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
