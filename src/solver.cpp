#include "solver.h"
#include "fast_tricks.h"

void PlayOrder::append_play(Card card) {
  if (all_cards_.contains(card)) {
    return;
  }
  all_cards_.add(card);
  assert(all_cards_.count() <= 13);
  cards_[card_count_++] = card;
}

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

Solver::Result Solver::solve() { return solve(0, game_.tricks_max()); }

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
        TRACE("tpn_cutoff", alpha, beta, score);
        return score;
      }
    }

    if (fast_tricks_enabled_) {
      int score;
      if (prune_fast_tricks(alpha, beta, score, winners_by_rank)) {
        TRACE("ft_cutoff", alpha, beta, score);
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

class LeadAnalyzer {
public:
  LeadAnalyzer(const Game &game, PlayOrder &order)
      : hands_(game.hands()),
        trumps_(game.trump_suit()),
        me_(game.next_seat()),
        lho_(right_seat(me_, 1)),
        pa_(right_seat(me_, 2)),
        rho_(right_seat(me_, 3)),
        valid_plays_(game.valid_plays_pruned()),
        order_(order),
        high_all_{NO_CARD, NO_CARD, NO_CARD, NO_CARD} {
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
        Cards c             = hands_.hand(seat).intersect(suit);
        high_[seat][suit]   = c.empty() ? NO_CARD : (int8_t)c.highest().index();
        low_[seat][suit]    = c.empty() ? NO_CARD : (int8_t)c.lowest().index();
        high_all_[suit]     = std::max(high_all_[suit], high_[seat][suit]);
        length_[seat][suit] = (int8_t)c.count();
      }
    }
  }

  void order_leads() {
    for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
      if (is_void(me_, suit)) {
        continue;
      }

      if (can_ruff(lho_, suit) || can_ruff(rho_, suit)) {
        bool overruff = can_ruff(pa_, suit) &&
                        high_[pa_][trumps_] > high_[lho_][trumps_] &&
                        high_[pa_][trumps_] > high_[rho_][trumps_];
        if (overruff) {
          play_low(suit);
        }
        continue;
      }

      if (high_[me_][suit] == high_all_[suit]) {
        play_high(suit);
      } else if (high_[pa_][suit] == high_all_[suit]) {
        play_low(suit);
      } else if (can_ruff(pa_, suit)) {
        play_low(suit);
      }
    }

    if (trumps_ != NO_TRUMP && can_est_length(trumps_)) {
      play_for_length(trumps_);
    }

    for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
      if (suit != trumps_ && can_est_length(suit)) {
        play_for_length(suit);
      }
    }

    order_.append_plays(valid_plays_, PlayOrder::LOW_TO_HIGH);
  }

private:
  bool is_void(Seat seat, Suit suit) { return high_[seat][suit] == NO_CARD; }

  bool can_ruff(Seat seat, Suit suit) {
    return suit != trumps_ && trumps_ != NO_TRUMP && is_void(seat, suit) &&
           !is_void(seat, trumps_);
  }

  bool can_est_length(Suit suit) {
    if (is_void(me_, suit)) {
      return false;
    }

    int our_voids = is_void(pa_, suit);
    int opp_voids = is_void(lho_, suit) + is_void(rho_, suit);
    int our_len   = std::max(length_[me_][suit], length_[pa_][suit]);
    int opp_len   = std::max(length_[lho_][suit], length_[pa_][suit]);
    return our_voids >= opp_voids && our_len > opp_len;
  }

  void play_low(Suit suit) {
    assert(!is_void(me_, suit));
    order_.append_play(Card(low_[me_][suit]));
  }

  void play_high(Suit suit) {
    assert(!is_void(me_, suit));
    order_.append_play(Card(high_[me_][suit]));
  }

  void play_for_length(Suit suit) {
    assert(!is_void(me_, suit));
    if (high_[pa_][suit] > high_[me_][suit]) {
      play_low(suit);
    } else {
      play_high(suit);
    }
  }

  static constexpr int8_t NO_CARD = -1;

  const Hands &hands_;
  const Suit   trumps_;
  const Seat   me_, lho_, pa_, rho_;
  const Cards  valid_plays_;
  PlayOrder   &order_;
  int8_t       high_[4][4];
  int8_t       low_[4][4];
  int8_t       high_all_[4];
  int8_t       length_[4][4];
};

void Solver::order_plays(PlayOrder &order) const {
  auto &trick = game_.current_trick();

  if (!trick.started()) {
    LeadAnalyzer poa(game_, order);
    poa.order_leads();
    return;
  }

  Cards valid_plays  = game_.valid_plays_pruned();
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

bool Solver::prune_fast_tricks(
    int alpha, int beta, int &score, Cards &winners_by_rank
) const {
  int fast_tricks;

  estimate_fast_tricks(
      game_.hands(),
      game_.next_seat(),
      game_.trump_suit(),
      fast_tricks,
      winners_by_rank
  );

  if (game_.next_seat() == NORTH || game_.next_seat() == SOUTH) {
    int lb = game_.tricks_taken_by_ns() + fast_tricks;
    if (lb >= beta) {
      score = lb;
      return true;
    }
  } else {
    int ub = game_.tricks_taken_by_ns() + game_.tricks_left() - fast_tricks;
    if (ub <= alpha) {
      score = ub;
      return true;
    }
  }

  return false;
}

void Solver::trace(const char *tag, int alpha, int beta, int score) {
  *trace_os_ << std::left;
  *trace_os_ << std::setw(7) << trace_lineno_ << ' ';
  *trace_os_ << std::setw(10) << tag << ' ';
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
