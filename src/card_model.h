#pragma once

#include <absl/container/flat_hash_map.h>

#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>

class ParseFailure : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

enum Suit {
  CLUBS,
  DIAMONDS,
  HEARTS,
  SPADES,
  NO_TRUMP,
};

std::istream &operator>>(std::istream &is, Suit &s);
std::ostream &operator<<(std::ostream &os, Suit s);

enum Rank {
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

std::istream &operator>>(std::istream &is, Rank &r);
std::ostream &operator<<(std::ostream &os, Rank r);

class Card {
public:
  Card() : rank_(RANK_2), suit_(CLUBS) {}
  Card(Rank r, Suit s) : rank_(r), suit_(s) { assert(s != NO_TRUMP); }
  Card(std::string_view s);

  Rank rank() const { return (Rank)rank_; }
  Suit suit() const { return (Suit)suit_; }

  std::string to_string() const;

private:
  uint8_t rank_;
  uint8_t suit_;

  friend std::istream &operator>>(std::istream &is, Card &c);
  friend std::ostream &operator<<(std::ostream &os, Card c);
};

inline bool operator==(const Card &lhs, const Card &rhs) {
  return lhs.rank() == rhs.rank() && lhs.suit() == rhs.suit();
}

class Cards {
public:
  class Iter {
  public:
    bool valid() const { return card_index_ >= 0; }
    Card card() const { return from_card_index(card_index_); }
    int  card_index() const { return card_index_; }

  private:
    Iter(int card_index) : card_index_(card_index) {
      assert(card_index >= 0 && card_index < 52);
    }

    Iter() : card_index_(-1) {}

    int card_index_;

    friend class Cards;
  };

  Cards() : bits_(0) {}
  Cards(uint64_t bits) : bits_(bits) { assert(!(bits & ~ALL_MASK)); }
  Cards(std::string_view s);

  uint64_t bits() const { return bits_; }

  void  add(Card c) { bits_ |= to_card_bit(c); }
  void  add_all(Cards c) { bits_ |= c.bits_; }
  void  remove(Card c) { bits_ &= ~to_card_bit(c); }
  void  add(int card_index) { bits_ |= to_card_bit(card_index); }
  void  remove(int card_index) { bits_ &= ~to_card_bit(card_index); }
  void  clear() { bits_ = 0; }
  bool  contains(Card c) const { return bits_ & to_card_bit(c); }
  int   count() const { return std::popcount(bits_); }
  Cards with(Card c) { return Cards(bits_ | to_card_bit(c)); }
  Cards complement() const { return Cards(~bits_ & ALL_MASK); }

  int top_ranks(Suit s) const {
    return std::countl_one(bits_ << ((3 - s) * 13 + 12));
  }

  Cards collapse_ranks(Cards to_collapse) const {
    if (!to_collapse.bits_) {
      return *this;
    }
    assert(disjoint(to_collapse));
    uint64_t bits = bits_;
    for (int suit_base = 0; suit_base < 52; suit_base += 13) {
      uint64_t s = SUIT_MASK << suit_base;
      uint64_t m = s;
      for (int j = 12; j > 0; j--) {
        bool should_collapse =
            ((uint64_t)1 << (suit_base + j)) & to_collapse.bits_;
        if (should_collapse) {
          bits = ((bits & m) << 1) | (bits & ~m);
        } else {
          m = (m >> 1) & s;
        }
      }
    }
    return Cards(bits);
  }

  static Card collapse_rank(Card card, Cards to_collapse) {
    assert(!to_collapse.contains(card));
    int suit_base = card.suit() * 13;
    int rank      = card.rank();
    for (int j = 12; j > card.rank(); j--) {
      bool should_collapse =
          ((uint64_t)1 << (suit_base + j)) & to_collapse.bits_;
      if (should_collapse) {
        rank++;
      }
    }
    return Card((Rank)rank, card.suit());
  }

  Cards remove_equivalent_ranks() const {
    uint64_t bits = 0;
    uint64_t m1   = 0b0000000000001000000000000100000000000010000000000001UL;
    uint64_t m2   = 0b0000000000010000000000001000000000000100000000000010UL;
    for (int i = 0; i < 12; i++) {
      bits |= (bits_ & m1) & ~((bits_ & m2) >> 1);
      m1 <<= 1;
      m2 <<= 1;
    }
    bits |= bits_ & m1;
    return Cards(bits);
  }

  Iter first() const {
    int k = std::countl_zero(bits_ << 12);
    return k < 64 ? Cards::Iter(51 - k) : Cards::Iter();
  }

  Iter next(Iter i) const {
    assert(i.valid());
    if (i.card_index_ <= 0) {
      // Beware! Shifting by > 63 bits is undefined behavior (and will not
      // necessarily zero out the value being shifted on some hardware).
      // Therefore, we *must* special case when bit_index_ is 0.
      return Cards::Iter();
    }
    int k = std::countl_zero(bits_ << (64 - i.card_index_));
    return k < 64 ? Cards::Iter(i.card_index_ - k - 1) : Cards::Iter();
  }

  bool empty() const { return !bits_; }

  Cards intersect(Cards c) const { return Cards(bits_ & c.bits_); }

  Cards intersect_suit(Suit s) const {
    return Cards(bits_ & (SUIT_MASK << ((uint64_t)s * 13)));
  }

  bool disjoint(Cards c) const { return intersect(c).empty(); }

  uint64_t bits_;

private:
  static int      to_card_index(Card c) { return c.rank() + c.suit() * 13; }
  static uint64_t to_card_bit(Card c) { return to_card_bit(to_card_index(c)); }
  static uint64_t to_card_bit(int card_index) {
    return ((uint64_t)1) << card_index;
  }

  static Card from_card_index(int card_index) {
    return Card((Rank)(card_index % 13), (Suit)(card_index / 13));
  }

  static const uint64_t SUIT_MASK = 0b1111111111111UL;
  static const uint64_t ALL_MASK =
      0b1111111111111111111111111111111111111111111111111111UL;

  friend bool operator==(Cards c1, Cards c2);

  template <typename H> friend H AbslHashValue(H h, const Cards &c) {
    return H::combine(std::move(h), c.bits_);
  }
};

std::istream &operator>>(std::istream &is, Cards &c);
std::ostream &operator<<(std::ostream &os, Cards c);

inline bool operator==(Cards c1, Cards c2) { return c1.bits_ == c2.bits_; }