#pragma once

#include <random>

#include "card_model.h"

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
std::istream &operator>>(std::istream &is, Seat &s);

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
  Trick()
      : trump_suit_(NO_TRUMP), lead_seat_(WEST), lead_suit_(NO_TRUMP),
        card_count_(0), winner_(0) {}

  int card_count() const { return card_count_; }
  Card card(int index) const { return cards_[index]; }
  Seat lead_seat() const { return lead_seat_; }
  Suit lead_suit() const { return lead_suit_; }
  Seat seat(int index) const { return right_seat(lead_seat_, index); }
  bool started() const { return card_count_ > 0; }
  bool finished() const { return card_count_ >= 4; }

  Seat winning_seat() const {
    assert(finished());
    return right_seat(lead_seat_, winner_);
  }

  void start_trick(Suit trump_suit, Seat lead_seat, Card c) {
    assert(card_count_ == 0);
    trump_suit_ = trump_suit;
    lead_seat_ = lead_seat;
    lead_suit_ = c.suit();
    cards_[0] = c;
    card_count_ = 1;
    winner_ = 0;
  }

  void continue_trick(Card c) {
    assert(card_count_ > 0 && card_count_ < 4);
    cards_[card_count_++] = c;
    if (finished()) {
      compute_winner();
    }
  }

  Card unplay() {
    assert(card_count_ > 0);
    winner_ = 0;
    return cards_[card_count_--];
  }

private:
  void compute_winner() {
    assert(finished());
    Card best = cards_[0];
    int winner = 0;
    for (int i = 1; i < 4; i++) {
      if (better_card(cards_[i], best)) {
        best = cards_[i];
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
  int card_count_;
  int winner_;
};

std::ostream &operator<<(std::ostream &os, const Trick &t);

class Game {
public:
  static Game random_deal(std::default_random_engine &random,
                          int cards_per_hand);

  Game(Contract contract, Cards hands[4]);

  Contract contract() const { return contract_; }
  Cards hand(Seat seat) const { return hands_[seat]; }
  Seat next_player() const { return next_player_; }
  Trick &current_trick() { return tricks_[trick_count_]; }
  const Trick &current_trick() const { return tricks_[trick_count_]; }
  int trick_count() const { return trick_count_; }
  int trick_max_count() const { return trick_max_count_; }
  int tricks_taken_by_ns() const { return tricks_taken_by_ns_; }

  void play(Card card);
  void unplay();

private:
  bool valid_play(Card c) const;

  Cards hands_[4];
  Contract contract_;
  Seat next_player_;
  Trick tricks_[13];
  int trick_count_;
  int trick_max_count_;
  int tricks_taken_by_ns_;

  friend std::ostream &operator<<(std::ostream &os, const Game &g);
};
