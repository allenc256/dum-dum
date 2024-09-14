#pragma once

#include <random>

#include "Cards.h"

enum Seat : uint8_t {
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

class Contract {
public:
  Contract(int level, Suit suit, Seat declarer)
      : level_(level), suit_(suit), declarer_(declarer) {
    assert(level > 0 && level <= 7);
  }

  int level() const { return level_; }
  Suit suit() const { return suit_; }
  Seat declarer() const { return declarer_; }

private:
  int level_;
  Suit suit_;
  Seat declarer_;
};

std::ostream &operator<<(std::ostream &os, Contract c);

class Trick {
public:
  Trick() : card_count_(0), lead_seat_(WEST) {}

  int card_count() const { return card_count_; }
  Card card(int index) const { return cards_[index]; }
  Seat lead_seat() const { return lead_seat_; }
  Suit lead_suit() const { return cards_[lead_seat_].suit(); }
  Seat seat(int index) const { return right_seat(lead_seat_, index); }
  bool started() const { return card_count_ > 0; }
  bool finished() const { return card_count_ >= 4; }

  void play_start(Seat lead_seat, Card c) {
    assert(card_count_ == 0);
    card_count_ = 1;
    cards_[0] = c;
    lead_seat_ = lead_seat;
  }

  void play_continue(Card c) {
    assert(card_count_ > 0 && card_count_ < 4);
    cards_[card_count_++] = c;
  }

  Card unplay() {
    assert(card_count_ > 0);
    return cards_[card_count_--];
  }

private:
  Card cards_[4];
  int card_count_;
  Seat lead_seat_;
};

std::ostream &operator<<(std::ostream &os, const Trick &t);

class Game {
public:
  static Game random_deal(std::default_random_engine &random);

  Cards cards(Seat seat) const { return cards_[seat]; }
  void play(Card card);
  void unplay();

private:
  Game(Contract contract, Cards cards[4]);

  bool valid_play(Card c) const;

  Trick &current_trick() { return tricks_[trick_count_]; }
  const Trick &current_trick() const { return tricks_[trick_count_]; }

  Seat winner(const Trick &t) const;

  Cards cards_[4];
  Contract contract_;
  Seat next_player_;
  Trick tricks_[13];
  int trick_count_;
  int trick_max_count_;
  __unused int tricks_taken_by_ns_;

  friend std::ostream &operator<<(std::ostream &os, const Game &g);
};
