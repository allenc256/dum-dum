#include "solver.h"

#include <picosha2.h>

void PlayOrder::append_plays(Cards cards, bool low_to_high) {
  cards.remove_all(all_cards_);
  if (cards.empty()) {
    return;
  }
  all_cards_.add_all(cards);
  assert(all_cards_.count() <= 13);
  if (low_to_high) {
    for (auto it = cards.iter_lowest(); it.valid();
         it      = cards.iter_higher(it)) {
      cards_[card_count_++] = it.card();
    }
  } else {
    for (auto it = cards.iter_highest(); it.valid();
         it      = cards.iter_lower(it)) {
      cards_[card_count_++] = it.card();
    }
  }
}

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
  best_play_.reset();
  assert(search_ply_ == 0);
  int tricks_taken_by_ns = solve_internal(alpha, beta, max_depth);
  assert(search_ply_ == 0);
  int tricks_taken_by_ew = game_.tricks_max() - tricks_taken_by_ns;
  assert(best_play_.has_value());
  return {
      .tricks_taken_by_ns = tricks_taken_by_ns,
      .tricks_taken_by_ew = tricks_taken_by_ew,
      .best_play          = *best_play_,
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
  TpnTable::Value tpn_value;

  if (game_.start_of_trick()) {
    if (tpn_table_enabled_) {
      lookup_tpn_value(max_depth, tpn_value);
      if (tpn_value.lower_bound >= beta ||
          tpn_value.lower_bound == tpn_value.upper_bound) {
        TRACE("prune", alpha, beta, tpn_value.lower_bound);
        if (search_ply_ == 0) {
          best_play_ = tpn_value.pv_play;
        }
        return tpn_value.lower_bound;
      }
      if (tpn_value.upper_bound <= alpha) {
        TRACE("prune", alpha, beta, tpn_value.upper_bound);
        return tpn_value.upper_bound;
      }
    }

    TRACE("start", alpha, beta, -1);
  }

  SearchState ss = {
      .maximizing = maximizing,
      .alpha      = alpha,
      .beta       = beta,
      .best_score = maximizing ? -1 : game_.tricks_max() + 1,
      .max_depth  = max_depth
  };
  search_all_cards(ss);

  if (game_.start_of_trick()) {
    TRACE("end", alpha, beta, ss.best_score);

    if (tpn_table_enabled_) {
      bool changed = false;
      if (ss.best_score < beta && ss.best_score < tpn_value.upper_bound) {
        tpn_value.upper_bound = ss.best_score;
        changed               = true;
      }
      if (ss.best_score > alpha && ss.best_score > tpn_value.lower_bound) {
        tpn_value.lower_bound = ss.best_score;
        changed               = true;
      }
      if (changed) {
        if (tpn_value.lower_bound == tpn_value.upper_bound) {
          tpn_value.pv_play = ss.best_play;
        }
        tpn_table_.upsert_value(max_depth, tpn_value);
      }
    }
  }

  if (search_ply_ == 0 && alpha < ss.best_score && ss.best_score < beta) {
    best_play_ = ss.best_play;
  }

  return ss.best_score;
}

void Solver::lookup_tpn_value(int max_depth, TpnTable::Value &value) {
  if (mini_solver_enabled_) {
    mini_solver_.solve(max_depth, value);
  } else {
    tpn_table_.lookup_value(max_depth, value);
  }
}

// void Solver::order_plays(PlayOrder &order) const {
//   switch (game_.current_trick().card_count()) {
//   case 0: order_plays_first_seat(order); break;
//   case 1: order_plays_second_seat(order); break;
//   case 2: order_plays_third_seat(order); break;
//   case 3: order_plays_fourth_seat(order); break;
//   }
//   assert(false);
// }

// void Solver::order_plays_first_seat(PlayOrder &order) const {
//   order.append_plays(game_.valid_plays_pruned(), PlayOrder::LOW_TO_HIGH);
// }

// static int card_to_strength(const Trick &trick, Card card) {
//   if (card.suit() == trick.trump_suit()) {
//     return card.rank() + 14;
//   } else if (card.suit() == trick.lead_suit()) {
//     return card.rank() + 1;
//   } else {
//     return 0;
//   }
// }

// void Solver::order_plays_second_seat(PlayOrder &order) const {
//   auto &trick = game_.current_trick();
//   Card  highest[4];
//   Card  overall_highest;
//   Card  my_highest      = highest[1];
//   Card  partner_highest = highest[3];
//   Cards valid_plays     = game_.valid_plays_pruned();

//   if (my_highest == overall_highest) {
//     //
//   }

//   // int max_str     = *std::max_element(strs, strs + 4);
//   // int my_str      = strs[1];
//   // int partner_str = strs[3];

//   // Seat  me                = game_.next_seat();
//   // Seat  opp               = right_seat(me, 1);
//   // Seat  partner           = right_seat(me, 2);
//   // Cards my_cards          = game_.hand(me);
//   // Cards opp_cards         = game_.hand(opp);
//   // Cards partner_cards     = game_.hand(partner);
//   // Card  my_best_card      = best_card(trick, my_cards);
//   // Card  opp_best_card     = best_card(trick, opp_cards);
//   // Card  partner_best_card = best_card(trick, partner_cards);
//   // int   curr_val          = card_to_strength(trick, trick.card(0));
//   // int   my_val            = card_to_strength(trick, my_best_card);
//   // int   opp_val           = card_to_strength(trick, opp_best_card);
//   // int   partner_val       = card_to_strength(trick, partner_best_card);

//   // int highest_val = curr_val;
//   // highest_val     = std::max(highest_val, my_val);
//   // highest_val     = std::max(highest_val, opp_val);
//   // highest_val     = std::max(highest_val, partner_val);

//   // Card right_card = best_card(game_.trump_suit(), game_.)

//   // if partner > opps
//   //   play my lowest first
//   // if me > opps
//   //   play my highest first
//   //
// }

bool Solver::search_all_cards(SearchState &s) {
  states_explored_++;

  const Trick &t        = game_.current_trick();
  Cards        my_plays = game_.valid_plays_pruned();

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
    if (child_tricks_by_ns > s.best_score) {
      s.best_score = child_tricks_by_ns;
      s.best_play  = c;
    }
    if (ab_pruning_enabled_) {
      s.alpha = std::max(s.alpha, s.best_score);
      if (s.best_score >= s.beta) {
        prune = true;
      }
    }
  } else {
    if (child_tricks_by_ns < s.best_score) {
      s.best_score = child_tricks_by_ns;
      s.best_play  = c;
    }
    if (ab_pruning_enabled_) {
      s.beta = std::min(s.beta, s.best_score);
      if (s.best_score <= s.alpha) {
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

  Game::State state;
  memset(&state, 0, sizeof(state));
  state = game.normalized_state();

  uint8_t  digest[32];
  uint8_t *in_begin = (uint8_t *)&state;
  uint8_t *in_end   = in_begin + sizeof(state);
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
