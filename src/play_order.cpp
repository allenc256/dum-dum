#include "play_order.h"
#include "card_model.h"

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

static Cards compute_sure_winners(
    const Trick &trick, const Hands &hands, Cards valid_plays
) {
  Card highest = trick.card(0);
  for (int i = 1; i < trick.card_count(); i++) {
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

  void compute_order() {
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

void order_plays(const Game &game, PlayOrder &order) {
  auto &trick = game.current_trick();

  if (!trick.started()) {
    LeadAnalyzer analyzer(game, order);
    analyzer.compute_order();
    return;
  }

  Cards valid_plays  = game.valid_plays_pruned();
  Cards sure_winners = compute_sure_winners(trick, game.hands(), valid_plays);
  order.append_plays(sure_winners, PlayOrder::LOW_TO_HIGH);

  if (trick.trump_suit() != NO_TRUMP) {
    Cards non_trump_losers =
        valid_plays.without_all(Cards::all(trick.trump_suit()));
    order.append_plays(non_trump_losers, PlayOrder::LOW_TO_HIGH);
  }

  order.append_plays(valid_plays, PlayOrder::LOW_TO_HIGH);
}