#pragma once

#include <array>
#include <iostream>
#include <optional>
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

  Trick(Suit trump_suit, Seat lead_seat, std::initializer_list<Card> cards)
      : Trick() {
    assert(cards.size() <= 4);
    bool first = true;
    for (Card card : cards) {
      if (first) {
        play_start(trump_suit, lead_seat, card);
        first = false;
      } else {
        play_continue(card);
      }
    }
  }

  bool has_card(int index) const {
    assert(index >= 0 && index < card_count_);
    return cards_[index].has_value();
  }

  Card card(int index) const {
    assert(has_card(index));
    return *cards_[index];
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
    assert(started() && !finished());
    return right_seat(lead_seat_, card_count_);
  }

  Seat winning_seat() const {
    assert(started());
    return right_seat(lead_seat_, winning_index());
  }

  Card winning_card() const {
    assert(started());
    int i = winning_index();
    assert(cards_[i].has_value());
    return *cards_[i];
  }

  int winning_index() const {
    assert(started());
    return winning_index_[card_count_ - 1];
  }

  Cards valid_plays(Cards hand) const {
    assert(!finished());
    if (!started()) {
      return hand;
    }
    Cards c = hand.intersect(lead_suit_);
    return c.empty() ? hand : c;
  }

  Cards winning_cards() const {
    assert(!finished());
    return card_count_ > 0 ? winning_cards_[card_count_ - 1] : Cards::all();
  }

  Cards all_cards() const {
    Cards c;
    for (int i = 0; i < card_count_; i++) {
      if (cards_[i].has_value()) {
        c.add(*cards_[i]);
      }
    }
    return c;
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
    cards_[card_count_] = c;
    card_count_++;
  }

  void play_null() {
    assert(card_count_ > 0 && card_count_ < 4);
    winning_index_[card_count_] = winning_index();
    winning_cards_[card_count_] = winning_cards();
    cards_[card_count_]         = std::nullopt;
    card_count_++;
  }

  std::optional<Card> unplay() {
    assert(card_count_ > 0);
    card_count_--;
    return cards_[card_count_];
  }

  bool won_by_rank() const {
    Card w = winning_card();
    for (int i = 0; i < 4; i++) {
      if (has_card(i)) {
        Card c = card(i);
        if (c.suit() == w.suit() && c.rank() < w.rank()) {
          return true;
        }
      }
    }
    return false;
  }

private:
  Cards compute_winning_cards(Card w) const {
    if (trump_suit_ == NO_TRUMP || w.suit() == trump_suit_) {
      return Cards::higher_ranking(w);
    } else {
      return Cards::higher_ranking(w).union_with(Cards::all(trump_suit_));
    }
  }

  Suit                trump_suit_;
  Seat                lead_seat_;
  Suit                lead_suit_;
  std::optional<Card> cards_[4];
  int                 card_count_;
  int                 winning_index_[4];
  Cards               winning_cards_[4];
};

std::ostream &operator<<(std::ostream &os, const Trick &t);

class Hands {
public:
  Hands() {}

  Hands(Cards west, Cards north, Cards east, Cards south)
      : hands_{west, north, east, south} {}

  Cards hand(Seat seat) const { return hands_[seat]; }
  void  add_card(Seat seat, Card c) { hands_[seat].add(c); }
  void  remove_card(Seat seat, Card c) { hands_[seat].remove(c); }

  bool all_same_size() const {
    int size = hands_[0].count();
    for (int i = 1; i < 4; i++) {
      if (hands_[i].count() != size) {
        return false;
      }
    }
    return true;
  }

  bool all_disjoint() const {
    for (int i = 0; i < 4; i++) {
      for (int j = i + 1; j < 4; j++) {
        if (!hands_[i].disjoint(hands_[j])) {
          return false;
        }
      }
    }
    return true;
  }

  Cards all_cards() const {
    Cards all;
    for (int i = 0; i < 4; i++) {
      all.add_all(hands_[i]);
    }
    return all;
  }

  template <typename H> friend H AbslHashValue(H h, const Hands &hands) {
    return H::combine(std::move(h), hands.hands_);
  }

  bool operator==(const Hands &hands) const = default;

private:
  std::array<Cards, 4> hands_;
};

class Game {
public:
  Game(Suit trump_suit, Seat first_lead_seat, const Hands &hands);

  Suit         trump_suit() const { return trump_suit_; }
  Seat         lead_seat() const { return lead_seat_; }
  Cards        hand(Seat seat) const { return hands_.hand(seat); }
  Seat         next_seat() const { return next_seat_; }
  Seat         next_seat(int i) const { return right_seat(next_seat_, i); }
  const Hands &hands() const { return hands_; }

  Trick       &current_trick() { return tricks_[tricks_taken_]; }
  const Trick &current_trick() const { return tricks_[tricks_taken_]; }

  Trick &last_finished_trick() {
    assert(tricks_taken_ > 0);
    return tricks_[tricks_taken_ - 1];
  }

  const Trick &last_finished_trick() const {
    assert(tricks_taken_ > 0);
    return tricks_[tricks_taken_ - 1];
  }

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
  bool start_of_trick() const { return !current_trick().started(); }

  void play(Card card);
  void play_null();
  void unplay();

  bool  valid_play(Card c) const;
  Cards valid_plays() const;

  const Hands &normalized_hands();

  Cards prune_equivalent_cards(Cards cards) const {
    return card_normalizer_.prune_equivalent(cards);
  }

  Card normalize_card(Card card) const {
    return card_normalizer_.normalize(card);
  }

  Card denormalize_card(Card card) const {
    return card_normalizer_.denormalize(card);
  }

private:
  void finish_play();

  Hands                hands_;
  Suit                 trump_suit_;
  Seat                 lead_seat_;
  Seat                 next_seat_;
  Trick                tricks_[14];
  int                  tricks_taken_;
  int                  tricks_max_;
  int                  tricks_taken_by_ns_;
  CardNormalizer       card_normalizer_;
  std::optional<Hands> norm_hands_stack_[14];

  friend std::ostream &operator<<(std::ostream &os, const Game &g);
};
