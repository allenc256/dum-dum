#pragma once

#include <array>
#include <iostream>
#include <optional>

#include "card_model.h"

enum Seat : int {
  WEST,
  NORTH,
  EAST,
  SOUTH,
  NO_SEAT,
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

class Hands {
public:
  Hands() {}
  Hands(Cards w, Cards n, Cards e, Cards s);
  Hands(std::string_view s);

  Cards hand(Seat seat) const;
  void  add_card(Seat seat, Card card);
  void  remove_card(Seat seat, Card card);

  bool  all_same_size() const;
  bool  all_disjoint() const;
  Cards all_cards() const;
  bool  contains_all(const Hands &other) const;

  Hands make_partition(Cards winners_by_rank) const;
  Hands normalize() const;

  template <typename H> friend H AbslHashValue(H h, const Hands &hands) {
    return H::combine(std::move(h), hands.hands_);
  }

  bool operator==(const Hands &hands) const = default;

  void pretty_print(std::ostream &os, Cards winners_by_rank) const;
  void compact_print(std::ostream &os, Cards winners_by_rank) const;

  std::string to_string() const;

  friend std::ostream &operator<<(std::ostream &os, const Hands &hands);
  friend std::istream &operator>>(std::istream &is, Hands &hands);

private:
  std::array<Cards, 4> hands_;
};

class Trick {
public:
  Trick();

  bool  started() const;
  bool  finished() const;
  Seat  lead_seat() const;
  Suit  lead_suit() const;
  Suit  trump_suit() const;
  Seat  seat(int index) const;
  Seat  next_seat() const;
  Card  card(int index) const;
  int   card_count() const;
  Cards all_cards() const;

  Seat  winning_seat() const;
  Card  winning_card() const;
  int   winning_index() const;
  Cards winning_cards() const;
  Cards winners_by_rank(const Hands &hands) const;

  Cards higher_cards(Card w) const;
  Card  highest_card(Cards hand) const;
  bool  is_higher_card(Card c1, Card c2) const;
  Card  higher_card(Card c1, Card c2) const;

  Cards valid_plays(Cards hand) const;
  void  play_start(Suit trump_suit, Seat lead_seat, Card c);
  void  play_continue(Card c);
  Card  unplay();

private:
  bool won_by_rank() const;
  int  card_value(Card c) const;

  Suit  trump_suit_;
  Seat  lead_seat_;
  Suit  lead_suit_;
  Card  cards_[4];
  int   card_count_;
  int   winning_index_[4];
  Cards winning_cards_[4];
};

std::ostream &operator<<(std::ostream &os, const Trick &t);

class Game {
public:
  Game(Suit trump_suit, Seat first_lead_seat, const Hands &hands);

  Suit         trump_suit() const;
  Seat         lead_seat() const;
  Cards        hand(Seat seat) const;
  const Hands &hands() const;
  Seat         next_seat() const;
  Seat         next_seat(int i) const;

  Trick       &current_trick();
  const Trick &current_trick() const;
  const Trick &last_trick() const;
  const Trick &trick(int i) const;

  bool started() const;
  bool finished() const;
  bool start_of_trick() const;
  int  tricks_taken() const;
  int  tricks_left() const;
  int  tricks_max() const;
  int  tricks_taken_by_ns() const;
  int  tricks_taken_by_ew() const;

  void play(Card card);
  void unplay();

  bool         valid_play(Card c) const;
  Cards        valid_plays_pruned() const;
  Cards        valid_plays_all() const;
  const Hands &normalized_hands() const;

  Card  normalize_card(Card card) const;
  Cards normalize_wbr(Cards winners_by_rank) const;
  Cards denormalize_wbr(Cards winners_by_rank) const;
  Card  denormalize_card(Card card) const;

  void pretty_print(std::ostream &os) const;

private:
  using HandsStack = std::array<std::optional<Hands>, 14>;

  Hands              hands_;
  Suit               trump_suit_;
  Seat               lead_seat_;
  Seat               next_seat_;
  Trick              tricks_[14];
  int                tricks_taken_;
  int                tricks_max_;
  int                tricks_taken_by_ns_;
  CardNormalizer     card_normalizer_;
  mutable HandsStack norm_hands_stack_;
};

std::ostream &operator<<(std::ostream &os, const Game &g);
