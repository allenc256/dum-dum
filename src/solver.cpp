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
  assert(search_ply_ == 0);
  Value value;
  solve_internal(alpha, beta, max_depth, value);
  assert(search_ply_ == 0);
  int tricks_taken_by_ns = value.score;
  int tricks_taken_by_ew = game_.tricks_max() - tricks_taken_by_ns;
  assert(value.best_play.has_value());
  return {
      .tricks_taken_by_ns = tricks_taken_by_ns,
      .tricks_taken_by_ew = tricks_taken_by_ew,
      .best_play          = *value.best_play,
      .level              = value.level,
  };
}

#define TRACE(tag, alpha, beta, score, level)                                  \
  if (trace_ostream_) {                                                        \
    trace(tag, alpha, beta, score, level);                                     \
  }

void Solver::solve_internal(int alpha, int beta, int max_depth, Value &value) {
  if (game_.finished()) {
    value.score = game_.tricks_taken_by_ns();
    TRACE("terminal", alpha, beta, value.score, value.level);
    return;
  }

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  TpnTable::Value tpn_value;

  if (game_.start_of_trick()) {
    if (solve_fast(alpha, beta, max_depth, value, tpn_value)) {
      return;
    } else {
      TRACE("start", alpha, beta, -1, AbsLevel());
    }
  }

  SearchState ss = {
      .maximizing = maximizing,
      .alpha      = alpha,
      .beta       = beta,
      .score      = maximizing ? -1 : game_.tricks_max() + 1,
      .max_depth  = max_depth
  };
  search_all_cards(ss);

  if (game_.start_of_trick()) {
    TRACE("end", alpha, beta, ss.score, ss.level);

    if (tpn_table_enabled_) {
      if (ss.score < beta) {
        tpn_value.upper_bound = to_int8_t(ss.score);
      }
      if (ss.score > alpha) {
        tpn_value.lower_bound = to_int8_t(ss.score);
      }
      if (tpn_value.lower_bound == tpn_value.upper_bound) {
        tpn_value.pv_play = ss.best_play;
      }
      tpn_value.level = ss.level;
      tpn_table_.upsert_value(max_depth, tpn_value);
    }
  }

  value.score = ss.score;
  value.level = ss.level;
  if (search_ply_ == 0 && alpha < ss.score && ss.score < beta) {
    value.best_play = ss.best_play;
  }
}

bool Solver::solve_fast(
    int alpha, int beta, int max_depth, Value &value, TpnTable::Value &tpn_value
) {
  assert(game_.start_of_trick());

  if (!tpn_table_enabled_) {
    return false;
  } else if (mini_solver_enabled_) {
    mini_solver_.solve(alpha, beta, max_depth, tpn_value);
  } else {
    tpn_table_.lookup_value(alpha, beta, max_depth, tpn_value);
  }

  if (tpn_value.lower_bound == tpn_value.upper_bound) {
    value.level     = tpn_value.level;
    value.score     = tpn_value.lower_bound;
    value.best_play = tpn_value.pv_play;
    TRACE("prune", alpha, beta, value.score, value.level);
    return true;
  }

  if (tpn_value.lower_bound >= beta) {
    value.level = tpn_value.level;
    value.score = tpn_value.lower_bound;
    TRACE("prune", alpha, beta, value.score, value.level);
    return true;
  }

  if (tpn_value.upper_bound <= alpha) {
    value.level = tpn_value.level;
    value.score = tpn_value.upper_bound;
    TRACE("prune", alpha, beta, value.score, value.level);
    return true;
  }

  return false;
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

  Value child_value;
  solve_internal(s.alpha, s.beta, s.max_depth, child_value);

  AbsLevel child_level = child_value.level;
  if (game_.start_of_trick()) {
    child_level.intersect(AbsLevel(game_.hands(), game_.last_finished_trick()));
  }

  bool prune = false;
  if (s.maximizing) {
    if (child_value.score > s.score) {
      s.score     = child_value.score;
      s.best_play = c;
    }
    if (ab_pruning_enabled_) {
      s.alpha = std::max(s.alpha, s.score);
      if (s.score >= s.beta) {
        s.level = child_level;
        prune   = true;
      }
    }
  } else {
    if (child_value.score < s.score) {
      s.score     = child_value.score;
      s.best_play = c;
    }
    if (ab_pruning_enabled_) {
      s.beta = std::min(s.beta, s.score);
      if (s.score <= s.alpha) {
        s.level = child_level;
        prune   = true;
      }
    }
  }

  if (!prune) {
    s.level.intersect(child_level);
  }

  search_ply_--;
  game_.unplay();
  return prune;
}

void Solver::trace(
    const char *tag, int alpha, int beta, int score, const AbsLevel &level
) {
  char line_buf[121], score_buf[3];
  if (score >= 0) {
    std::snprintf(score_buf, sizeof(score_buf), "%2d", score);
  } else {
    std::strcpy(score_buf, "  ");
  }
  std::snprintf(
      line_buf,
      sizeof(line_buf),
      "%-7llu %-8s %2d %2d %2d %s",
      trace_lineno_,
      tag,
      alpha,
      beta,
      game_.tricks_taken_by_ns(),
      score_buf
  );
  *trace_ostream_ << line_buf << ' ' << level;
  for (int i = 0; i < game_.tricks_taken(); i++) {
    *trace_ostream_ << " " << game_.trick(i);
  }
  *trace_ostream_ << std::endl;
  trace_lineno_++;
}
