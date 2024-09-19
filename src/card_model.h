#pragma once

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

  Rank rank() const { return rank_; }
  Suit suit() const { return suit_; }

  std::string to_string() const;

private:
  Rank rank_;
  Suit suit_;

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
    int card_index() const { return card_index_; }

  private:
    Iter(int card_index) : card_index_(card_index) {
      assert(card_index >= 0 && card_index < 52);
    }

    Iter() : card_index_(-1) {}

    int card_index_;

    friend class Cards;
  };

  Cards() : bits_(0) {}
  Cards(uint64_t bits) : bits_(bits) { assert(!(bits & INVALID_MASK)); }
  Cards(std::string_view s);

  uint64_t bits() const { return bits_; }

  void add(Card c) { bits_ |= to_card_bit(c); }
  void remove(Card c) { bits_ &= ~to_card_bit(c); }
  void add(int card_index) { bits_ |= to_card_bit(card_index); }
  void remove(int card_index) { bits_ &= ~to_card_bit(card_index); }
  void clear() { bits_ = 0; }

  bool contains(Card c) const { return bits_ & to_card_bit(c); }
  int count() const { return std::popcount(bits_); }

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
  static int to_card_index(Card c) { return c.rank() + c.suit() * 13; }
  static uint64_t to_card_bit(Card c) { return to_card_bit(to_card_index(c)); }
  static uint64_t to_card_bit(int card_index) {
    return ((uint64_t)1) << card_index;
  }

  static Card from_card_index(int card_index) {
    return Card((Rank)(card_index % 13), (Suit)(card_index / 13));
  }

  static const uint64_t SUIT_MASK = 0b1111111111111UL;
  static const uint64_t INVALID_MASK =
      ~0b1111111111111111111111111111111111111111111111111111UL;

  friend bool operator==(Cards c1, Cards c2);
};

std::istream &operator>>(std::istream &is, Cards &c);
std::ostream &operator<<(std::ostream &os, Cards c);

inline bool operator==(Cards c1, Cards c2) { return c1.bits_ == c2.bits_; }