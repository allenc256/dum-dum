#pragma once

#include "game_model.h"

class AbsTrick {
public:
  enum State { STARTING, STARTED, FINISHING, FINISHED };

  AbsTrick()
      : card_count_(0),
        lead_seat_(NO_SEAT),
        trump_suit_(NO_TRUMP),
        winning_seat_(NO_SEAT),
        can_finish_{false},
        state_(STARTING) {}

  State state() const { return state_; }

  Suit lead_suit() const {
    assert(state_ != STARTING);
    return cards_[lead_seat_].suit();
  }

  Seat lead_seat() const {
    assert(state_ != STARTING);
    return lead_seat_;
  }

  bool can_finish(Seat seat) const {
    assert(state_ == FINISHING);
    return can_finish_[right_seat_diff(lead_seat_, seat)];
  }

  void finish(Seat seat) {
    assert(state_ == FINISHING && can_finish_[seat]);
    winning_seat_ = seat;
    state_        = FINISHED;
  }

  Seat unfinish() {
    assert(state_ == FINISHED);
    Seat s        = winning_seat_;
    winning_seat_ = NO_SEAT;
    state_        = FINISHING;
    return s;
  }

  void play_starting_card(Seat lead_seat, Suit trump_suit, Card card) {
    assert(state_ == STARTING);
    cards_[0]   = card;
    card_count_ = 1;
    trump_suit_ = trump_suit;
    lead_seat_  = lead_seat;
    state_      = STARTED;
  }

  void play_continuing_card(Card card) {
    assert(state_ == STARTED);
    cards_[card_count_] = card;
    card_count_++;
    if (card_count_ == 4) {
      state_ = FINISHING;
      compute_can_finish();
    }
  }

  Card unplay() {
    assert(state_ == STARTED || state_ == FINISHING);
    card_count_--;
    Card c = cards_[card_count_];
    if (card_count_ == 0) {
      state_ = STARTING;
    } else {
      state_ = STARTED;
    }
    return c;
  }

private:
  void compute_can_finish() {
    Card best      = cards_[0];
    Suit lead_suit = cards_[lead_seat_].suit();
    for (int i = 1; i < 4; i++) {
      if (is_higher_card(cards_[i], best, lead_suit)) {
        best = cards_[i];
      }
    }
    for (int i = 0; i < 4; i++) {
      can_finish_[i] = (cards_[i] == best);
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

  Card  cards_[4];
  int   card_count_;
  Seat  lead_seat_;
  Suit  trump_suit_;
  Seat  winning_seat_;
  bool  can_finish_[4];
  State state_;
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

  AbsCards(std::string_view s);

  Cards high_cards() const { return high_cards_; }
  int   low_cards(Suit suit) const { return low_cards_[suit]; }

  int size() const {
    int n = high_cards_.count();
    for (int i = 0; i < 4; i++) {
      n += low_cards_[i];
    }
    return n;
  }

  void add(Card c) {
    if (c.rank() == RANK_UNKNOWN) {
      low_cards_[c.suit()]++;
    } else {
      assert(!high_cards_.contains(c));
      high_cards_.add(c);
    }
  }

  void remove(Card c) {
    if (c.rank() == RANK_UNKNOWN) {
      assert(low_cards_[c.suit()] > 0);
      low_cards_[c.suit()]--;
    } else {
      assert(high_cards_.contains(c));
      high_cards_.remove(c);
    }
  }

  bool contains(Suit suit) const {
    return !high_cards_.intersect(suit).empty() || low_cards_[suit] > 0;
  }

  AbsCards intersect(Suit suit) const {
    Cards  hc    = high_cards_.intersect(suit);
    int8_t lc[4] = {0};
    lc[suit]     = low_cards_[suit];
    return AbsCards(hc, lc);
  }

  friend bool operator==(const AbsCards &c1, const AbsCards &c2) {
    if (c1.high_cards_ != c2.high_cards_) {
      return false;
    }
    for (int i = 0; i < 4; i++) {
      if (c1.low_cards_[i] != c2.low_cards_[i]) {
        return false;
      }
    }
    return true;
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

private:
  friend class Iter;

  Cards  high_cards_;
  int8_t low_cards_[4];
};

std::ostream &operator<<(std::ostream &os, const AbsCards &c);
std::istream &operator>>(std::istream &is, AbsCards &c);

class AbsGame {
public:
  AbsGame(Suit trump_suit, Seat initial_lead_seat, AbsCards hands[4])
      : trump_suit_(trump_suit),
        next_seat_(initial_lead_seat),
        tricks_taken_(0),
        tricks_taken_by_ns_(0),
        tricks_max_(hands[0].size()) {
    for (int i = 1; i < 4; i++) {
      if (hands[i].size() != tricks_max_) {
        throw std::runtime_error("hands must be same size");
      }
    }

    for (int i = 0; i < 4; i++) {
      hands_[i] = hands[i];
    }
  }

  bool            finished() const { return tricks_taken_ == tricks_max_; }
  Seat            next_seat() const { return next_seat_; }
  AbsTrick::State trick_state() const { return tricks_[tricks_taken_].state(); }
  int             tricks_taken() const { return tricks_taken_; }
  int             tricks_taken_by_ns() const { return tricks_taken_by_ns_; }

  AbsCards valid_plays() const {
    const AbsTrick &t = tricks_[tricks_taken_];
    const AbsCards &h = hands_[next_seat_];
    if (t.state() == AbsTrick::STARTING) {
      return h;
    } else {
      assert(t.state() == AbsTrick::STARTED);
      if (h.contains(t.lead_suit())) {
        return h.intersect(t.lead_suit());
      } else {
        return h;
      }
    }
  }

  void play(Card c) {
    assert(tricks_taken_ < tricks_max_);
    AbsTrick &t = tricks_[tricks_taken_];
    if (t.state() == AbsTrick::STARTING) {
      t.play_starting_card(next_seat_, trump_suit_, c);
    } else if (t.state() == AbsTrick::STARTED) {
      t.play_continuing_card(c);
    } else {
      assert(false);
    }
    hands_[next_seat_].remove(c);
    next_seat_ = right_seat(next_seat_);
  }

  void unplay() {
    AbsTrick &t = tricks_[tricks_taken_];
    assert(t.state() == AbsTrick::STARTED || t.state() == AbsTrick::FINISHING);
    Card c     = t.unplay();
    next_seat_ = left_seat(next_seat_);
    hands_[next_seat_].add(c);
  }

  bool can_finish_trick(Seat seat) const {
    return tricks_[tricks_taken_].can_finish(seat);
  }

  void finish_trick(Seat seat) {
    tricks_[tricks_taken_].finish(seat);
    tricks_taken_++;
    if (seat == NORTH || seat == SOUTH) {
      tricks_taken_by_ns_++;
    }
    next_seat_ = seat;
  }

  void unfinish_trick() {
    assert(tricks_taken_ > 0);
    assert(tricks_[tricks_taken_].state() == AbsTrick::STARTING);
    tricks_taken_--;
    AbsTrick &t = tricks_[tricks_taken_];
    Seat      s = t.unfinish();
    if (s == NORTH || s == SOUTH) {
      tricks_taken_by_ns_--;
    }
    next_seat_ = t.lead_seat();
  }

private:
  Suit     trump_suit_;
  Seat     next_seat_;
  AbsCards hands_[4];
  AbsTrick tricks_[14];
  int      tricks_taken_;
  int      tricks_taken_by_ns_;
  int      tricks_max_;
};
