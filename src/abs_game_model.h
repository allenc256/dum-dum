#pragma once

#include "game_model.h"

class AbsTrick {
public:
  AbsTrick()
      : card_count_(0),
        lead_seat_(NO_SEAT),
        trump_suit_(NO_TRUMP),
        is_poss_winning_{false},
        chosen_winning_seat_(NO_SEAT) {}

  bool started() const { return card_count_ > 0; }
  bool finished() const { return card_count_ == 4; }

  Suit lead_suit() const {
    assert(started());
    return cards_[lead_seat_].suit();
  }

  bool is_poss_winning(Seat seat) const {
    assert(finished());
    return is_poss_winning_[right_seat_diff(lead_seat_, seat)];
  }

  Seat chosen_winning_seat() const {
    assert(chosen_winning_seat_ != NO_SEAT);
    return chosen_winning_seat_;
  }

  void choose_winning_seat(Seat seat) {
    assert(finished());
    assert(is_poss_winning_[seat]);
    assert(chosen_winning_seat_ != NO_SEAT);
    chosen_winning_seat_ = seat;
  }

  void unchoose_winning_seat() {
    assert(finished());
    assert(chosen_winning_seat_ != NO_SEAT);
    chosen_winning_seat_ = NO_SEAT;
  }

  void play_starting_card(Seat lead_seat, Suit trump_suit, Card card) {
    assert(!started());
    cards_[0]   = card;
    card_count_ = 1;
    trump_suit_ = trump_suit;
    lead_seat_  = lead_seat;
  }

  void play_continuing_card(Card card) {
    assert(started() && !finished());
    cards_[card_count_] = card;
    card_count_++;
    if (card_count_ == 4) {
      compute_winning_seats();
    }
  }

  void unplay() {
    assert(started());
    card_count_--;
  }

private:
  void compute_winning_seats() {
    Card best      = cards_[0];
    Suit lead_suit = cards_[lead_seat_].suit();
    for (int i = 1; i < 4; i++) {
      if (is_higher_card(cards_[i], best, lead_suit)) {
        best = cards_[i];
      }
    }
    for (int i = 0; i < 4; i++) {
      is_poss_winning_[i] = (cards_[i] == best);
    }
  }

  bool is_higher_card(Card c1, Card c2, Suit lead_suit) const {
    if (c1.suit() == trump_suit_) {
      if (c2.suit() == trump_suit_) {
        return is_higher_rank(c1.rank(), c2.rank());
      } else {
        return true;
      }
    } else if (c2.suit() == trump_suit_) {
      return false;
    } else if (c1.suit() == lead_suit) {
      assert(c2.suit() == lead_suit);
      return is_higher_rank(c1.rank(), c2.rank());
    } else {
      assert(c2.suit() == lead_suit);
      return false;
    }
  }

  bool is_higher_rank(Rank r1, Rank r2) const {
    if (r1 == RANK_UNKNOWN) {
      return false;
    } else if (r2 == RANK_UNKNOWN) {
      return true;
    } else {
      return r1 > r2;
    }
  }

  Card cards_[4];
  int  card_count_;
  Seat lead_seat_;
  Suit trump_suit_;
  bool is_poss_winning_[4];
  Seat chosen_winning_seat_;
};

class AbsCards {
public:
  AbsCards() : low_cards_{0} {}

  AbsCards(Cards high_cards, const int8_t low_cards[4])
      : high_cards_(high_cards) {
    for (int i = 0; i < 4; i++) {
      low_cards_[i] = low_cards[i];
    }
  }

  Cards high_cards() const { return high_cards_; }
  int   low_cards(Suit suit) const { return low_cards_[suit]; }

  int size() const {
    int n = high_cards_.count();
    for (int i = 0; i < 4; i++) {
      n += low_cards_[i];
    }
    return n;
  }

  class Iter {
  public:
    Iter(const AbsCards &parent)
        : parent_(parent),
          high_it_(parent_.high_cards_.iter_highest()),
          low_it_((Suit)(FIRST_SUIT - 1)) {
      advance_low_it();
    }

    bool valid() const { return high_it_.valid() || low_it_ <= LAST_SUIT; }

    Card card() const {
      assert(valid());
      if (high_it_.valid()) {
        return high_it_.card();
      } else {
        return Card(RANK_UNKNOWN, low_it_);
      }
    }

    void next() {
      assert(valid());
      if (high_it_.valid()) {
        advance_high_it();
      } else {
        advance_low_it();
      }
    }

  private:
    void advance_high_it() {
      high_it_ = parent_.high_cards_.iter_lower(high_it_);
    }

    void advance_low_it() {
      assert(low_it_ <= LAST_SUIT);
      do {
        low_it_++;
      } while (low_it_ <= LAST_SUIT && parent_.low_cards_[low_it_] == 0);
    }

    const AbsCards &parent_;
    Cards::Iter     high_it_;
    Suit            low_it_;
  };

  Iter iter() const { return Iter(*this); }
  // AbsPlays valid_plays(const AbsTrick &trick) const {
  //   if (!trick.started()) {
  //     bool lc[4];
  //     for (int i = 0; i < 4; i++) {
  //       lc[i] = low_cards_[i] > 0;
  //     }
  //     return AbsPlays(high_cards_, lc);
  //   } else {
  //     Suit  s     = trick.lead_suit();
  //     Cards hc    = high_cards_.intersect(s);
  //     bool  lc[4] = {false};
  //     lc[s]       = low_cards_[s] > 0;
  //     return AbsPlays(hc, lc);
  //   }
  // }

private:
  friend class Iter;

  Cards  high_cards_;
  int8_t low_cards_[4];
};

// class AbsGame {
// public:
//   AbsGame(Suit trump_suit, Seat initial_lead_seat, AbsHand hands[4])
//       : trump_suit_(trump_suit),
//         initial_lead_seat_(initial_lead_seat),
//         tricks_taken_(0),
//         tricks_taken_by_ns_(0) {
//     int hand_size = hands[0].size();
//     for (int i = 1; i < 4; i++) {
//       if (hands[i].size() != hand_size) {
//         throw std::runtime_error("hands must be same size");
//       }
//     }

//     for (int i = 0; i < 4; i++) {
//       hands_[i] = hands[i];
//     }
//   }

//   struct ValidPlays {
//     Cards high_cards;
//     bool  low_cards[4];
//   };

//   ValidPlays valid_plays() const {
//     const AbsTrick &t = tricks_[tricks_taken_];
//     if (!t.started()) {
//       // return { .high_cards=hands_[next_seat_].high_cards(), .low_c};
//     }
//   }

// private:
//   Suit     trump_suit_;
//   Seat     initial_lead_seat_;
//   Seat     next_seat_;
//   AbsHand  hands_[4];
//   AbsTrick tricks_[14];
//   int      tricks_taken_;
//   int      tricks_taken_by_ns_;
//   int      tricks_max_;
// };
