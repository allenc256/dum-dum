#include "solver.h"

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
      nodes_explored_(0),
      tpn_table_(game_),
      trace_os_(nullptr),
      trace_lineno_(0) {
  enable_all_optimizations(true);
}

Solver::~Solver() {}

Solver::Result Solver::solve() { return solve(-1, game_.tricks_max() + 1); }

Solver::Result Solver::solve(int alpha, int beta) {
  Cards winners_by_rank;
  int   tricks_taken_by_ns = solve_internal(alpha, beta, winners_by_rank);
  int   tricks_taken_by_ew = game_.tricks_max() - tricks_taken_by_ns;
#ifndef NDEBUG
  if (tpn_table_enabled_) {
    tpn_table_.check_invariants();
  }
#endif
  return {
      .tricks_taken_by_ns = tricks_taken_by_ns,
      .tricks_taken_by_ew = tricks_taken_by_ew,
      .winners_by_rank    = winners_by_rank,
  };
}

#define TRACE(tag, alpha, beta, score)                                         \
  if (trace_os_) {                                                             \
    trace(tag, alpha, beta, score);                                            \
  }

int Solver::solve_internal(int alpha, int beta, Cards &winners_by_rank) {
  if (game_.finished()) {
    TRACE("terminal", alpha, beta, game_.tricks_taken_by_ns());
    return game_.tricks_taken_by_ns();
  }

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;

  if (game_.start_of_trick()) {
    if (tpn_table_enabled_) {
      int score;
      if (tpn_table_.lookup(alpha, beta, score, winners_by_rank)) {
        TRACE("lookup", alpha, beta, score);
        return score;
      }
    }

    TRACE("start", alpha, beta, -1);
  }

  int best_score = maximizing ? -1 : game_.tricks_max() + 1;
  search_all_cards(alpha, beta, best_score, winners_by_rank);

  if (game_.start_of_trick()) {
    TRACE("end", alpha, beta, best_score);

    if (tpn_table_enabled_) {
      int lower_bound = game_.tricks_taken_by_ns();
      int upper_bound = game_.tricks_taken_by_ns() + game_.tricks_left();
      if (best_score < beta) {
        upper_bound = best_score;
      }
      if (best_score > alpha) {
        lower_bound = best_score;
      }
      tpn_table_.insert(winners_by_rank, lower_bound, upper_bound);
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

static void add_last_trick_wbr(const Game &game, Cards &winners_by_rank) {
  if (game.start_of_trick()) {
    Cards last_trick_wbr = game.last_trick().winners_by_rank(game.hands());
    winners_by_rank.add_all(last_trick_wbr);
  }
}

void Solver::search_all_cards(
    int alpha, int beta, int &best_score, Cards &winners_by_rank
) {
  nodes_explored_++;

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
          add_last_trick_wbr(game_, winners_by_rank);
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
          add_last_trick_wbr(game_, winners_by_rank);
          game_.unplay();
          return;
        }
      }
    }

    winners_by_rank.add_all(child_winners_by_rank);
    add_last_trick_wbr(game_, winners_by_rank);
    game_.unplay();
  }
}

void Solver::trace(const char *tag, int alpha, int beta, int score) {
  *trace_os_ << std::left;
  *trace_os_ << std::setw(7) << trace_lineno_ << ' ';
  *trace_os_ << std::setw(8) << tag << ' ';
  *trace_os_ << std::right;
  *trace_os_ << game_.hands();
  int max_len = game_.tricks_max() * 4 + 15;
  int cur_len = game_.hands().all_cards().count() + 15;
  *trace_os_ << std::setw(max_len - cur_len + 1) << ' ';
  *trace_os_ << std::setw(2) << alpha << ' ';
  *trace_os_ << std::setw(2) << beta << ' ';
  *trace_os_ << std::setw(2) << game_.tricks_taken_by_ns() << ' ';
  if (score >= 0) {
    *trace_os_ << std::setw(2) << score << ' ';
  } else {
    *trace_os_ << "   ";
  }
  for (int i = 0; i < game_.tricks_taken(); i++) {
    *trace_os_ << " " << game_.trick(i);
  }
  *trace_os_ << '\n';
  trace_lineno_++;
}
