#pragma once

#include <random>

#include "card_model.h"

enum Seat {
  WEST,
  NORTH,
  EAST,
  SOUTH,
};

inline Seat left_seat(Seat s) { return (Seat)((s + 3) & 3); }
inline Seat right_seat(Seat s) { return (Seat)((s + 1) & 3); }

inline Seat left_seat(Seat s, int i) {
  assert(i >= 0);
  return (Seat)((s + i * 3) & 3);
}

inline Seat right_seat(Seat s, int i) {
  assert(i >= 0);
  return (Seat)((s + i) & 3);
}

std::ostream &operator<<(std::ostream &os, Seat s);
std::istream &operator>>(std::istream &is, Seat &s);

class Trick {
public:
  Trick()
      : trump_suit_(NO_TRUMP),
        lead_seat_(WEST),
        lead_suit_(NO_TRUMP),
        card_count_(0),
        winner_(0) {}

  Card card(int index) const {
    assert(index >= 0 && index < 4);
    return cards_[index];
  }

  int  card_count() const { return card_count_; }
  bool started() const { return card_count_ > 0; }
  bool finished() const { return card_count_ >= 4; }

  Seat lead_seat() const {
    assert(started());
    return lead_seat_;
  }

  Suit lead_suit() const {
    assert(started());
    return lead_suit_;
  }

  Seat seat(int index) const {
    assert(index >= 0 && index < 4);
    return right_seat(lead_seat_, index);
  }

  Seat next_seat() const {
    assert(started());
    if (finished()) {
      return right_seat(lead_seat_, winner_);
    } else {
      return right_seat(lead_seat_, card_count_);
    }
  }

  void play_start(Suit trump_suit, Seat lead_seat, Card c) {
    assert(card_count_ == 0);
    trump_suit_ = trump_suit;
    lead_seat_  = lead_seat;
    lead_suit_  = c.suit();
    cards_[0]   = c;
    card_count_ = 1;
    winner_     = 0;
  }

  void play_continue(Card c) {
    assert(card_count_ > 0 && card_count_ < 4);
    cards_[card_count_++] = c;
    if (finished()) {
      compute_winner();
    }
  }

  Card unplay() {
    assert(card_count_ > 0);
    winner_ = 0;
    card_count_--;
    return cards_[card_count_];
  }

private:
  void compute_winner() {
    assert(finished());
    Card best   = cards_[0];
    int  winner = 0;
    for (int i = 1; i < 4; i++) {
      if (better_card(cards_[i], best)) {
        best   = cards_[i];
        winner = i;
      }
    }
    winner_ = winner;
  }

  bool better_card(Card c, Card winner) {
    if (trump_suit_ == NO_TRUMP) {
      return c.suit() == lead_suit_ && c.rank() > winner.rank();
    }
    if (winner.suit() == trump_suit_) {
      return c.suit() == trump_suit_ && c.rank() > winner.rank();
    }
    if (c.suit() == trump_suit_) {
      return true;
    }
    if (c.suit() == lead_suit_) {
      return c.rank() > winner.rank();
    }
    return false;
  }

  Suit trump_suit_;
  Seat lead_seat_;
  Suit lead_suit_;
  Card cards_[4];
  int  card_count_;
  int  winner_;
};

std::ostream &operator<<(std::ostream &os, const Trick &t);

class Game {
public:
  static Game
  random_deal(std::default_random_engine &random, int cards_per_hand);

  Game(Suit trump_suit, Seat declarer, Cards hands[4]);

  Suit  trump_suit() const { return trump_suit_; }
  Seat  declarer() const { return declarer_; }
  Cards hand(Seat seat) const { return hands_[seat]; }
  Seat  next_seat() const { return next_seat_; }

  Trick       &current_trick() { return tricks_[tricks_taken_]; }
  const Trick &current_trick() const { return tricks_[tricks_taken_]; }

  const Trick &trick(int i) const {
    assert(i < tricks_taken_);
    return tricks_[i];
  }

  bool started() const {
    return current_trick().started() || tricks_taken_ > 0;
  }

  int tricks_taken() const { return tricks_taken_; }
  int tricks_left() const { return tricks_max_ - tricks_taken_; }
  int tricks_max() const { return tricks_max_; }
  int tricks_taken_by_ns() const { return tricks_taken_by_ns_; }
  int tricks_taken_by_ew() const { return tricks_taken_ - tricks_taken_by_ns_; }
  bool finished() const { return tricks_taken_ == tricks_max_; }

  void play(Card card);
  void unplay();

  bool  valid_play(Card c) const;
  Cards valid_plays() const;

private:
  Cards hands_[4];
  Suit  trump_suit_;
  Seat  declarer_;
  Seat  next_seat_;
  Trick tricks_[14];
  int   tricks_taken_;
  int   tricks_max_;
  int   tricks_taken_by_ns_;

  friend std::ostream &operator<<(std::ostream &os, const Game &g);
};
