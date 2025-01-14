#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "parser.h"

enum Suit : int8_t {
  CLUBS,
  DIAMONDS,
  HEARTS,
  SPADES,
  NO_TRUMP,
};

constexpr Suit FIRST_SUIT = CLUBS;
constexpr Suit LAST_SUIT  = SPADES;

Suit operator++(Suit &s, int);
Suit operator--(Suit &s, int);
Suit parse_suit(std::string_view str);
Suit parse_suit(Parser &parser);

enum Rank : int8_t {
  RANK_2,
  RANK_3,
  RANK_4,
  RANK_5,
  RANK_6,
  RANK_7,
  RANK_8,
  RANK_9,
  TEN,
  JACK,
  QUEEN,
  KING,
  ACE
};

Rank operator++(Rank &r, int);
Rank operator--(Rank &r, int);
Rank parse_rank(std::string_view str);
Rank parse_rank(Parser &parser);

class Card {
public:
  Card();
  Card(int card_index);
  Card(Rank r, Suit s);
  Card(Parser &parser);
  Card(std::string_view s);
  Card(const char *s);

  Rank rank() const;
  Suit suit() const;
  int  index() const { return index_; }
  bool operator==(const Card &other) const = default;

private:
  uint8_t index_;
};

class Cards {
public:
  template <bool is_low_to_high> class Iterator {
  public:
    Card operator*() const;
    void operator++();
    bool operator==(const Iterator &it) const;

    static Iterator begin(uint64_t bits);
    static Iterator end(uint64_t bits);

  private:
    Iterator(uint64_t bits, int card_index);

    friend class Cards;

    uint64_t bits_;
    int      card_index_;
  };

  template <bool is_low_to_high> class Iterable {
  public:
    Iterator<is_low_to_high> begin() const;
    Iterator<is_low_to_high> end() const;

  private:
    friend class Cards;

    Iterable(uint64_t bits) : bits_(bits) {}

    uint64_t bits_;
  };

  Cards();
  Cards(uint64_t bits);
  Cards(Parser &parser);
  Cards(std::string_view s);
  Cards(std::initializer_list<std::string_view> cards);

  uint64_t bits() const;
  bool     empty() const;
  void     add(Card c);
  void     add_all(Cards c);
  void     remove(Card c);
  void     remove_all(Cards c);
  bool     contains(Card c) const;
  bool     contains_all(Cards c) const;
  int      count() const;
  Cards    with(Card c) const;
  Cards    with_all(Cards c) const;
  Cards    complement() const;
  bool     disjoint(Cards c) const;
  Cards    intersect(Cards c) const;
  Cards    without_all(Cards c) const;
  Cards    without_lower(Rank rank) const;
  Cards    intersect(Suit s) const;
  Cards    normalize(Cards removed) const;
  Cards    normalize_wbr(Cards removed) const;
  Cards    prune_equivalent(Cards removed) const;
  bool     operator==(const Cards &c) const = default;

  Iterable<true>  low_to_high() const;
  Iterable<false> high_to_low() const;

  Card lowest() const;
  Card highest() const;
  Card lowest_equivalent(Card card, Cards removed) const;

  static Cards all();
  static Cards all(Suit s);
  static Cards higher_ranking(Card card);
  static Cards higher_ranking_or_eq(Card card);
  static Cards lower_ranking(Card card);

private:
  uint64_t bits_;

  template <typename H> friend H AbslHashValue(H h, const Cards &c) {
    return H::combine(std::move(h), c.bits_);
  }
};

class SuitNormalizer {
public:
  SuitNormalizer();

  Rank normalize(Rank rank) const;
  Rank denormalize(Rank rank) const;
  void remove(Rank rank);
  void add(Rank rank);

private:
  uint64_t norm_map_;
  uint64_t denorm_map_;
};

class CardNormalizer {
public:
  Card normalize(Card card) const;
  Card denormalize(Card card) const;

  Cards normalize(Cards cards) const;
  Cards normalize_wbr(Cards cards) const;
  Cards denormalize_wbr(Cards cards) const;
  Cards prune_equivalent(Cards cards) const;

  void remove(Card card);
  void add(Card card);
  void remove_all(Cards cards);

private:
  Cards          removed_;
  SuitNormalizer norm_[4];
};

// ----------------------
// Implementation Details
// ----------------------

inline Suit operator++(Suit &s, int) { return (Suit)((int8_t &)s)++; }
inline Suit operator--(Suit &s, int) { return (Suit)((int8_t &)s)--; }

template <>
struct std::formatter<Suit> : public std::formatter<std::string_view> {
  auto format(Suit suit, auto &ctx) const {
    return formatter<std::string_view>::format(to_string(suit), ctx);
  }

  const std::string_view &to_string(Suit suit) const;
};

inline Rank operator++(Rank &r, int) { return (Rank)((int8_t &)r)++; }
inline Rank operator--(Rank &r, int) { return (Rank)((int8_t &)r)--; }

template <> struct std::formatter<Rank> {
  constexpr auto parse(auto &ctx) { return ctx.begin(); }

  auto format(Rank rank, auto &ctx) const {
    return std::format_to(ctx.out(), "{}", to_char(rank));
  }

  char to_char(Rank rank) const;
};

template <> struct std::formatter<Card> {
  constexpr auto parse(auto &ctx) { return ctx.begin(); }

  auto format(Card card, auto &ctx) const {
    return std::format_to(ctx.out(), "{}{}", card.rank(), card.suit());
  }
};

template <bool is_low_to_high>
inline Card Cards::Iterator<is_low_to_high>::operator*() const {
  return Card(card_index_);
}

template <bool is_low_to_high>
inline void Cards::Iterator<is_low_to_high>::operator++() {
  assert(card_index_ >= 0);
  if constexpr (is_low_to_high) {
    int k = std::countr_zero(bits_ >> (card_index_ + 1));
    if (k < 64) {
      card_index_ += k + 1;
    } else {
      card_index_ = -1;
    }
  } else {
    if (card_index_ <= 0) {
      // Beware! Shifting by > 63 bits is undefined behavior (and will not
      // necessarily zero out the value being shifted on some hardware).
      // Therefore, we *must* special case when bit_index_ is 0.
      card_index_ = -1;
    } else {
      int k = std::countl_zero(bits_ << (64 - card_index_));
      if (k < 64) {
        card_index_ -= k + 1;
      } else {
        card_index_ = -1;
      }
    }
  }
}

template <bool is_low_to_high>
inline bool Cards::Iterator<is_low_to_high>::operator==(const Iterator &it
) const {
  return card_index_ == it.card_index_;
}

template <bool is_low_to_high>
inline Cards::Iterator<is_low_to_high>
Cards::Iterator<is_low_to_high>::begin(uint64_t bits) {
  if constexpr (is_low_to_high) {
    int k = std::countr_zero(bits);
    return k < 64 ? Iterator(bits, k) : end(bits);
  } else {
    int k = std::countl_zero(bits << 12);
    return k < 64 ? Iterator(bits, 51 - k) : end(bits);
  }
}

template <bool is_low_to_high>
inline Cards::Iterator<is_low_to_high>
Cards::Iterator<is_low_to_high>::end(uint64_t bits) {
  return Iterator(bits, -1);
}

template <bool is_low_to_high>
inline Cards::Iterator<is_low_to_high>::Iterator(uint64_t bits, int card_index)
    : bits_(bits),
      card_index_(card_index) {}

template <bool is_low_to_high>
inline Cards::Iterator<is_low_to_high>
Cards::Iterable<is_low_to_high>::begin() const {
  return Iterator<is_low_to_high>::begin(bits_);
}

template <bool is_low_to_high>
inline Cards::Iterator<is_low_to_high>
Cards::Iterable<is_low_to_high>::end() const {
  return Iterator<is_low_to_high>::end(bits_);
}

template <> struct std::formatter<Cards> {
  constexpr auto parse(auto &ctx) { return ctx.begin(); }

  auto format(Cards cards, auto &ctx) const {
    auto out = ctx.out();
    for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
      if (suit != LAST_SUIT) {
        out = std::format_to(out, ".");
      }
      for (Card card : cards.intersect(suit).high_to_low()) {
        out = std::format_to(out, "{}", card.rank());
      }
    }
    return out;
  }
};
