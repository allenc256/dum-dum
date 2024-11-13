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
    for (Card c : cards.low_to_high()) {
      cards_[card_count_++] = c;
    }
  } else {
    for (Card c : cards.high_to_low()) {
      cards_[card_count_++] = c;
    }
  }
}

Solver::Solver(Game g)
    : game_(g),
      states_explored_(0),
      tpn_table_(game_),
      trace_ostream_(nullptr),
      trace_lineno_(0) {
  enable_all_optimizations(true);
}

Solver::~Solver() {}

Solver::Result Solver::solve() { return solve(-1, game_.tricks_max() + 1); }

Solver::Result Solver::solve(int alpha, int beta) {
  Cards winners_by_rank;
  int   tricks_taken_by_ns = solve_internal(alpha, beta, winners_by_rank);
  int   tricks_taken_by_ew = game_.tricks_max() - tricks_taken_by_ns;
  return {
      .tricks_taken_by_ns = tricks_taken_by_ns,
      .tricks_taken_by_ew = tricks_taken_by_ew,
      .winners_by_rank    = winners_by_rank,
  };
}

#define TRACE(tag, alpha, beta, tricks_taken_by_ns)                            \
  if (trace_ostream_) {                                                        \
    trace(tag, alpha, beta, tricks_taken_by_ns);                               \
  }

int Solver::solve_internal(int alpha, int beta, Cards &winners_by_rank) {
  if (game_.finished()) {
    TRACE("terminal", alpha, beta, game_.tricks_taken_by_ns());
    return game_.tricks_taken_by_ns();
  }

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;
  TpnTable::Value tpn_value;

  if (game_.start_of_trick()) {
    if (tpn_table_enabled_) {
      tpn_table_.lookup_value(game_.tricks_max(), tpn_value);
      if (tpn_value.lower_bound >= beta ||
          tpn_value.lower_bound == tpn_value.upper_bound) {
        TRACE("prune", alpha, beta, tpn_value.lower_bound);
        return tpn_value.lower_bound;
      }
      if (tpn_value.upper_bound <= alpha) {
        TRACE("prune", alpha, beta, tpn_value.upper_bound);
        return tpn_value.upper_bound;
      }
    }

    TRACE("start", alpha, beta, -1);
  }

  int best_score = maximizing ? -1 : game_.tricks_max() + 1;
  search_all_cards(alpha, beta, best_score, winners_by_rank);

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
        tpn_table_.upsert_value(game_.tricks_max(), tpn_value);
      }
    }
  }

  return best_score;
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
    int alpha, int beta, int &best_score, Cards &winners_by_rank
) {
  states_explored_++;

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;

  PlayOrder order;
  order_plays(order);

  for (Card c : order) {
    game_.play(c);

    Cards child_winners_by_rank;
    int   child_score = solve_internal(alpha, beta, child_winners_by_rank);

    if (maximizing) {
      if (child_score > best_score) {
        best_score = child_score;
      }
      if (ab_pruning_enabled_) {
        alpha = std::max(alpha, best_score);
        if (best_score >= beta) {
          winners_by_rank = child_winners_by_rank;
          game_.unplay();
          return;
        }
      }
    } else {
      if (child_score < best_score) {
        best_score = child_score;
      }
      if (ab_pruning_enabled_) {
        beta = std::min(beta, best_score);
        if (best_score <= alpha) {
          winners_by_rank = child_winners_by_rank;
          game_.unplay();
          return;
        }
      }
    }

    winners_by_rank.add_all(child_winners_by_rank);
    if (game_.start_of_trick()) {
      Cards trick_winners_by_rank =
          game_.last_trick().winners_by_rank(game_.hands());
      winners_by_rank.add_all(trick_winners_by_rank);
    }

    game_.unplay();
  }
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
