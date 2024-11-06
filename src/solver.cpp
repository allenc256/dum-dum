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

  int  best_score = maximizing ? -1 : game_.tricks_max() + 1;
  Card best_play;
  search_all_cards(max_depth, alpha, beta, best_score, best_play);

  if (game_.start_of_trick()) {
    TRACE("end", alpha, beta, best_score);

    if (tpn_table_enabled_) {
      bool changed = false;
      if (best_score < beta && best_score < tpn_value.upper_bound) {
        tpn_value.upper_bound = best_score;
        changed               = true;
      }
      if (best_score > alpha && best_score > tpn_value.lower_bound) {
        tpn_value.lower_bound = best_score;
        changed               = true;
      }
      if (changed) {
        if (tpn_value.lower_bound == tpn_value.upper_bound) {
          tpn_value.pv_play = best_play;
        }
        tpn_table_.upsert_value(max_depth, tpn_value);
      }
    }
  }

  if (search_ply_ == 0 && alpha < best_score && best_score < beta) {
    best_play_ = best_play;
  }

  return best_score;
}

void Solver::lookup_tpn_value(int max_depth, TpnTable::Value &value) {
  if (mini_solver_enabled_) {
    mini_solver_.solve(max_depth, value);
  } else {
    tpn_table_.lookup_value(max_depth, value);
  }
}

static Cards compute_sure_winners(
    const Trick &trick, const Hands &hands, Cards valid_plays
) {
  Card highest = trick.card(0);
  for (int i = 1; i < trick.card_count(); i++) {
    assert(trick.has_card(i));
    highest = trick.higher_card(trick.card(i), highest);
  }
  for (int i = trick.card_count() + 1; i < 4; i++) {
    highest = trick.higher_card(
        trick.highest_card(hands.hand(trick.seat(i))), highest
    );
  }

  return trick.higher_cards(highest).intersect(valid_plays);
}

void Solver::order_plays(PlayOrder &order) const {
  auto &trick       = game_.current_trick();
  Cards valid_plays = game_.valid_plays_pruned();

  if (!trick.started()) {
    order.append_plays(valid_plays, PlayOrder::LOW_TO_HIGH);
    return;
  }

  // 2nd seat:
  //
  // - 0XXX (lead always wins)
  //   - low
  //
  // - 10XX (only I can win)
  // - X01X (only I can win)
  //   - high
  // - X0X1 (opps always lose)
  //   - one low + one high
  //
  // - 1X0X (opponents always win)
  //   - low
  // - X10X (finesse)
  //   - cover?
  // - XX01 (self-finesse?)
  //   - low
  //
  // - 1XX0 (partner must win)
  //   - low
  // - X1X0 (opps always lose)
  //   - one low + one high
  // - XX10 (self-finesse?)
  //   - low

  // 3rd seat:
  //
  // - 0XXX (lead always wins)
  // - X0XX (2nd seat always wins)
  //   - low
  //
  // - 1X0X (lead already winning)
  //   - low
  // - X10X (only I can win)
  // - XX01 (only I can win)
  //   - high
  //
  // - 1XX0 (opp will win)
  // - X1X0 (opp will win)
  //   - low
  // - XX10 (finesse)
  //   - cover?

  // 4th seat:
  //
  // - 0XXX
  // - X0XX
  // - XX0X
  //   - low
  //
  // - 1XX0 (only I can win)
  //   - high
  // - X1X0 (partner has won)
  //   - low
  // - XX10 (only I can win)
  //   - high

  Cards sure_winners = compute_sure_winners(trick, game_.hands(), valid_plays);
  order.append_plays(sure_winners, PlayOrder::LOW_TO_HIGH);

  if (trick.trump_suit() != NO_TRUMP) {
    Cards non_trump_losers =
        valid_plays.without_all(Cards::all(trick.trump_suit()));
    order.append_plays(non_trump_losers, PlayOrder::LOW_TO_HIGH);
  }

  order.append_plays(valid_plays, PlayOrder::LOW_TO_HIGH);
}

void Solver::search_all_cards(
    int max_depth, int alpha, int beta, int &best_score, Card &best_play
) {
  states_explored_++;

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;

  PlayOrder order;
  order_plays(order);

  for (Card c : order) {
    game_.play(c);
    search_ply_++;

    int child_score = solve_internal(alpha, beta, max_depth);
    if (maximizing) {
      if (child_score > best_score) {
        best_score = child_score;
        best_play  = c;
      }
      if (ab_pruning_enabled_) {
        alpha = std::max(alpha, best_score);
        if (best_score >= beta) {
          search_ply_--;
          game_.unplay();
          return;
        }
      }
    } else {
      if (child_score < best_score) {
        best_score = child_score;
        best_play  = c;
      }
      if (ab_pruning_enabled_) {
        beta = std::min(beta, best_score);
        if (best_score <= alpha) {
          search_ply_--;
          game_.unplay();
          return;
        }
      }
    }

    search_ply_--;
    game_.unplay();
  }
}

static void print_sha256_hash(std::ostream &os, Game &game) {
  if (!game.start_of_trick()) {
    os << std::setw(16) << ' ';
    return;
  }

  Game::State state;
  memset(&state, 0, sizeof(state));
  state = game.normalized_state();

  uint8_t  digest[32];
  uint8_t *in_begin = (uint8_t *)&state;
  uint8_t *in_end   = in_begin + sizeof(state);
  picosha2::hash256(in_begin, in_end, digest, digest + 32);

  os << std::hex << std::setfill('0');
  os << std::setw(8) << *(uint32_t *)&digest[0];
  os << std::setw(8) << *(uint32_t *)&digest[1];
  os << std::dec << std::setfill(' ');
}

void Solver::trace(
    const char *tag, int alpha, int beta, int tricks_taken_by_ns
) {
  *trace_ostream_ << std::left;
  *trace_ostream_ << std::setw(7) << trace_lineno_ << ' ';
  *trace_ostream_ << std::setw(8) << tag << ' ';
  print_sha256_hash(*trace_ostream_, game_);
  *trace_ostream_ << std::setw(2) << alpha << ' ';
  *trace_ostream_ << std::setw(2) << beta << ' ';
  *trace_ostream_ << std::setw(2) << game_.tricks_taken_by_ns() << ' ';
  if (tricks_taken_by_ns >= 0) {
    *trace_ostream_ << std::setw(2) << tricks_taken_by_ns;
  } else {
    *trace_ostream_ << "  ";
  }
  for (int i = 0; i < game_.tricks_taken(); i++) {
    *trace_ostream_ << ' ' << game_.trick(i);
  }
  *trace_ostream_ << '\n';
  trace_lineno_++;
}
