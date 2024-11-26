#include "fast_tricks.h"

class MiniSolver {
public:
  MiniSolver(const Hands &hands, Seat my_seat, Suit trump_suit)
      : trump_suit_(trump_suit),
        removed_(hands.all_cards().complement()),
        tricks_taken_(0) {
    me_  = my_seat;
    lho_ = right_seat(me_, 1);
    pa_  = right_seat(me_, 2);
    rho_ = right_seat(me_, 3);

    hands_[me_]  = hands.hand(me_);
    hands_[lho_] = hands.hand(lho_);
    hands_[pa_]  = hands.hand(pa_);
    hands_[rho_] = hands.hand(rho_);

    for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
      for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
        Cards cards = hands_[seat].intersect(suit);
        if (cards.empty()) {
          high_card_[seat][suit] = NO_CARD;
          low_card_[seat][suit]  = NO_CARD;
        } else {
          high_card_[seat][suit] = (int8_t)cards.highest().index();
          low_card_[seat][suit]  = (int8_t)cards.lowest().index();
        }
      }
    }

    Cards trumps = trump_suit == NO_TRUMP ? Cards() : Cards::all(trump_suit);
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      non_trumps_[seat] = hands_[seat].without_all(trumps).count();
    }
  }

  void solve(int &fast_tricks, Cards &winners_by_rank) {
    bool progress;
    do {
      progress = false;
      if (trump_suit_ != NO_TRUMP) {
        while (try_trick(trump_suit_, true)) {
          progress = true;
        }
      }

      for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
        if (suit != trump_suit_) {
          while (try_trick(suit, true)) {
            progress = true;
          }
        }
      }

      if (trump_suit_ != NO_TRUMP) {
        if (try_trick(trump_suit_, false)) {
          continue;
        }
      }

      for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
        if (suit != trump_suit_) {
          if (try_trick(suit, false)) {
            progress = true;
            break;
          }
        }
      }

      // TODO: handle ruffs
    } while (progress);

#ifndef NDEBUG
    check_invariants();
#endif

    fast_tricks     = tricks_taken_;
    winners_by_rank = winners_by_rank_;
  }

private:
  bool is_void(Seat seat, Suit suit) const {
    return high_card_[seat][suit] == NO_CARD;
  }

  bool is_blocked(Suit suit, bool end_in_hand) const {
    if (end_in_hand) {
      return high_card_[me_][suit] < low_card_[pa_][suit];
    } else {
      return high_card_[pa_][suit] < low_card_[me_][suit];
    }
  }

  bool can_ruff(Seat seat, Suit suit) {
    return trump_suit_ != NO_TRUMP && suit != trump_suit_ &&
           is_void(seat, suit) && !is_void(seat, trump_suit_);
  }

  bool has_sufficient_discards(Seat seat, Suit suit) {
    return suit == trump_suit_ || !is_void(seat, suit) || non_trumps_[seat] > 0;
  }

  bool has_high_card(Seat seat, Suit suit) {
    auto hc = high_card_[seat][suit];
    return hc >= high_card_[0][suit] && hc >= high_card_[1][suit] &&
           hc >= high_card_[2][suit] && hc >= high_card_[3][suit];
  }

  bool try_trick(Suit suit, bool end_in_hand) {
    Seat dest = end_in_hand ? me_ : pa_;

    if (is_void(me_, suit)) {
      return false;
    }
    if (!has_high_card(dest, suit)) {
      return false;
    }
    if (is_blocked(suit, end_in_hand)) {
      return false;
    }
    if (can_ruff(lho_, suit) || can_ruff(rho_, suit)) {
      return false;
    }
    if (end_in_hand && !has_sufficient_discards(pa_, suit)) {
      return false;
    }

    bool lho_void = is_void(lho_, suit);
    bool rho_void = is_void(rho_, suit);
    bool pa_void  = is_void(pa_, suit);

    bool wins_by_rank = !lho_void || !rho_void || !pa_void;
    if (wins_by_rank) {
      Card c = Card(high_card_[dest][suit]);
      c      = hands_[dest].lowest_equivalent(c, removed_);
      winners_by_rank_.add_all(Cards::higher_ranking_or_eq(c));
    }

    if (!lho_void) {
      play_low(lho_, suit);
    }
    if (!rho_void) {
      play_low(rho_, suit);
    }

    if (end_in_hand) {
      play_high(me_, suit);
      play_low_or_discard(pa_, suit);
    } else {
      play_low(me_, suit);
      play_high(pa_, suit);

      std::swap(me_, pa_);
      std::swap(lho_, rho_);
    }

    tricks_taken_++;

    return true;
  }

  void play_high(Seat seat, Suit suit) {
    assert(!is_void(seat, suit));

    Card c = Card(high_card_[seat][suit]);

    hands_[seat].remove(c);
    removed_.add(c);

    if (high_card_[seat][suit] == low_card_[seat][suit]) {
      high_card_[seat][suit] = NO_CARD;
      low_card_[seat][suit]  = NO_CARD;
    } else {
      Cards remaining = hands_[seat].intersect(suit);
      assert(!remaining.empty());
      high_card_[seat][suit] = (int8_t)remaining.highest().index();
    }

    if (suit != trump_suit_) {
      non_trumps_[seat]--;
      assert(non_trumps_[seat] >= 0);
    }
  }

  void play_low(Seat seat, Suit suit) {
    assert(!is_void(seat, suit));

    Card c = Card(low_card_[seat][suit]);

    hands_[seat].remove(c);
    removed_.add(c);

    if (high_card_[seat][suit] == low_card_[seat][suit]) {
      high_card_[seat][suit] = NO_CARD;
      low_card_[seat][suit]  = NO_CARD;
    } else {
      Cards remaining = hands_[seat].intersect(suit);
      assert(!remaining.empty());
      low_card_[seat][suit] = (int8_t)remaining.lowest().index();
    }

    if (suit != trump_suit_) {
      non_trumps_[seat]--;
      assert(non_trumps_[seat] >= 0);
    }
  }

  void play_discard(Seat seat, Suit suit) {
    assert(non_trumps_[seat] > 0);

    Suit best_suit = NO_TRUMP;

    for (Suit disc_suit = FIRST_SUIT; disc_suit <= LAST_SUIT; disc_suit++) {
      if (suit == disc_suit) {
        continue;
      }
      if (disc_suit == trump_suit_ && suit != trump_suit_) {
        continue;
      }
      if (is_void(seat, disc_suit)) {
        continue;
      }
      if (best_suit == NO_TRUMP ||
          Card(low_card_[seat][disc_suit]).rank() <
              Card(low_card_[seat][best_suit]).rank()) {
        best_suit = disc_suit;
      }
    }

    assert(best_suit != NO_TRUMP);
    play_low(seat, best_suit);
  }

  void play_low_or_discard(Seat seat, Suit suit) {
    if (is_void(seat, suit)) {
      play_discard(seat, suit);
    } else {
      play_low(seat, suit);
    }
  }

#ifndef NDEBUG
  void check_invariants() {
    Cards all_cards;
    for (int i = 0; i < 4; i++) {
      all_cards.add_all(hands_[i]);
    }
    assert(removed_ == all_cards.complement());

    for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
      for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
        Cards cards = hands_[seat].intersect(suit);
        if (cards.empty()) {
          assert(high_card_[seat][suit] == NO_CARD);
          assert(low_card_[seat][suit] == NO_CARD);
        } else {
          assert(high_card_[seat][suit] == (int8_t)cards.highest().index());
          assert(low_card_[seat][suit] == (int8_t)cards.lowest().index());
        }
      }
    }

    Cards trumps = trump_suit_ == NO_TRUMP ? Cards() : Cards::all(trump_suit_);
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      assert(non_trumps_[seat] == hands_[seat].without_all(trumps).count());
    }
  }
#endif

  static constexpr int8_t NO_CARD = -1;

  Suit   trump_suit_;
  Cards  hands_[4];
  Cards  removed_;
  int8_t high_card_[4][4];
  int8_t low_card_[4][4];
  int    non_trumps_[4];
  Cards  winners_by_rank_;
  int    tricks_taken_;
  Seat   me_, pa_, lho_, rho_;
};

void estimate_fast_tricks(
    const Hands &hands,
    Seat         my_seat,
    Suit         trump_suit,
    int         &fast_tricks,
    Cards       &winners_by_rank
) {
  MiniSolver solver(hands, my_seat, trump_suit);
  solver.solve(fast_tricks, winners_by_rank);
}
