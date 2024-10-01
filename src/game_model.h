#pragma once

#include <random>

#include "card_model.h"

enum Seat : int {
  WEST,
  NORTH,
  EAST,
  SOUTH,
};

constexpr Seat FIRST_SEAT = WEST;
constexpr Seat LAST_SEAT  = SOUTH;

inline Seat operator++(Seat &s, int) { return (Seat)((int &)s)++; }
inline Seat operator--(Seat &s, int) { return (Seat)((int &)s)--; }

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
        card_count_(0) {}

  Card card(int index) const {
    assert(index >= 0 && index < 4);
    return cards_[index];
  }

  int  card_count() const { return card_count_; }
  bool started() const { return card_count_ > 0; }
  bool finished() const { return card_count_ >= 4; }

  bool skipped(int index) const {
    assert(index >= 0 && index < card_count_);
    return skipped_[index];
  }

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
    assert(started() && !finished());
    return right_seat(lead_seat_, card_count_);
  }

  Seat winning_seat() const {
    assert(started());
    return right_seat(lead_seat_, winning_index());
  }

  Card winning_card() const {
    assert(started());
    return cards_[winning_index()];
  }

  int winning_index() const {
    assert(started());
    return winning_index_[card_count_ - 1];
  }

  Cards all_cards() const {
    Cards c;
    for (int i = 0; i < card_count_; i++) {
      c.add(cards_[i]);
    }
    return c;
  }

  Cards valid_plays(Cards hand) const {
    assert(!finished());
    if (!started()) {
      return hand;
    }
    Cards c = hand.intersect_suit(lead_suit_);
    return c.empty() ? hand : c;
  }

  Cards winning_cards() const {
    assert(!finished());
    return card_count_ > 0 ? winning_cards_[card_count_ - 1] : Cards::all();
  }

  void play_start(Suit trump_suit, Seat lead_seat, Card c) {
    assert(card_count_ == 0);
    trump_suit_       = trump_suit;
    lead_seat_        = lead_seat;
    lead_suit_        = c.suit();
    cards_[0]         = c;
    card_count_       = 1;
    winning_cards_[0] = compute_winning_cards(c);
    winning_index_[0] = 0;
    skipped_[0]       = false;
  }

  void play_continue(Card c) {
    assert(card_count_ > 0 && card_count_ < 4);
    if (winning_cards().contains(c)) {
      winning_index_[card_count_] = card_count_;
      winning_cards_[card_count_] = compute_winning_cards(c);
    } else {
      winning_index_[card_count_] = winning_index();
      winning_cards_[card_count_] = winning_cards();
    }
    cards_[card_count_]   = c;
    skipped_[card_count_] = false;
    card_count_++;
  }

  void skip_continue() {
    assert(card_count_ > 0 && card_count_ < 4);
    skipped_[card_count_]       = true;
    winning_index_[card_count_] = winning_index();
    winning_cards_[card_count_] = winning_cards();
    card_count_++;
  }

  Card unplay() {
    assert(card_count_ > 0);
    assert(!skipped_[card_count_ - 1]);
    card_count_--;
    return cards_[card_count_];
  }

  void unskip() {
    assert(card_count_ > 0);
    assert(skipped_[card_count_ - 1]);
    card_count_--;
  }

private:
  Cards compute_winning_cards(Card w) const {
    if (trump_suit_ == NO_TRUMP || w.suit() == trump_suit_) {
      return Cards::higher_ranks(w);
    } else {
      return Cards::higher_ranks(w).union_with(Cards::all(trump_suit_));
    }
  }

  Suit  trump_suit_;
  Seat  lead_seat_;
  Suit  lead_suit_;
  Card  cards_[4];
  int   card_count_;
  int   winning_index_[4];
  Cards winning_cards_[4];
  bool  skipped_[4];
};

std::ostream &operator<<(std::ostream &os, const Trick &t);

class Game {
public:
  static Game
  random_deal(std::default_random_engine &random, int cards_per_hand);

  Game(Suit trump_suit, Seat first_lead_seat, Cards hands[4]);

  Suit  trump_suit() const { return trump_suit_; }
  Seat  lead_seat() const { return lead_seat_; }
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

  Cards ignorable_cards() const {
    Cards in_play_cards = current_trick().all_cards();
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      in_play_cards.add_all(hands_[seat]);
    }
    return in_play_cards.complement();
  }

private:
  Cards hands_[4];
  Suit  trump_suit_;
  Seat  lead_seat_;
  Seat  next_seat_;
  Trick tricks_[14];
  int   tricks_taken_;
  int   tricks_max_;
  int   tricks_taken_by_ns_;

  friend std::ostream &operator<<(std::ostream &os, const Game &g);
};
