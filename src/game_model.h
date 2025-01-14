#pragma once

#include <array>
#include <format>
#include <iostream>
#include <optional>

#include "card_model.h"
#include "parser.h"

enum Seat : int {
  WEST,
  NORTH,
  EAST,
  SOUTH,
  NO_SEAT,
};

constexpr Seat FIRST_SEAT = WEST;
constexpr Seat LAST_SEAT  = SOUTH;

Seat operator++(Seat &s, int);
Seat operator--(Seat &s, int);
Seat left_seat(Seat s);
Seat right_seat(Seat s);
Seat left_seat(Seat s, int i);
Seat right_seat(Seat s, int i);
Seat parse_seat(std::string_view str);
Seat parse_seat(Parser &parser);

class Hands {
public:
  Hands() {}
  Hands(Cards w, Cards n, Cards e, Cards s);
  Hands(std::string_view s);
  Hands(Parser &parser);

  Cards hand(Seat seat) const;
  void  add_card(Seat seat, Card card);
  void  remove_card(Seat seat, Card card);

  bool  all_same_size() const;
  bool  all_disjoint() const;
  Cards all_cards() const;
  bool  contains_all(const Hands &other) const;
  bool  operator==(const Hands &hands) const = default;

  Hands make_partition(Cards winners_by_rank) const;
  Hands normalize() const;

  template <typename H> friend H AbslHashValue(H h, const Hands &hands) {
    return H::combine(std::move(h), hands.hands_);
  }

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

// ----------------------
// Implementation Details
// ----------------------

template <>
struct std::formatter<Seat> : public std::formatter<std::string_view> {
  auto format(Seat seat, auto &ctx) const {
    return formatter<std::string_view>::format(to_string(seat), ctx);
  }

  std::string_view to_string(Seat seat) const;
};

template <> struct std::formatter<Trick> {
  constexpr auto parse(auto &ctx) { return ctx.begin(); }

  auto format(const Trick &trick, auto &ctx) const {
    auto out = ctx.out();
    if (!trick.started()) {
      out = std::format_to(out, "{}", '-');
    } else {
      for (int i = 0; i < trick.card_count(); i++) {
        out = std::format_to(out, "{}", trick.card(i));
      }
      if (trick.finished()) {
        out = std::format_to(out, " {}", trick.winning_seat());
      }
    }
    return out;
  }
};

template <> struct std::formatter<Hands> {
  constexpr auto parse(const auto &ctx) { return ctx.begin(); }

  auto format(const Hands &hands, auto &ctx) const {
    auto out = ctx.out();
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      if (seat != FIRST_SEAT) {
        out = std::format_to(out, "{}", '/');
      }
      Cards hand = hands.hand(seat);
      for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
        if (suit != LAST_SUIT) {
          out = std::format_to(out, "{}", '.');
        }
        for (Card card : hand.intersect(suit).high_to_low()) {
          out = std::format_to(out, "{}", card.rank());
        }
      }
    }
    return out;
  }
};
