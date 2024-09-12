#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <ostream>

enum Seat : uint8_t {
  WEST,
  NORTH,
  EAST,
  SOUTH,
};

enum Suit : uint8_t {
  CLUBS,
  DIAMONDS,
  HEARTS,
  SPADES,
};

enum Rank : uint8_t {
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

class Card {
 public:
  Card(Rank r, Suit s) : rank_(r), suit_(s) {}
  Card(std::string_view utf8_str);

  Rank rank() const { return rank_; }
  Suit suit() const { return suit_; }

 private:
  Rank rank_;
  Suit suit_;

  friend class Cards;
};

class Cards {
 public:
  class Iter {
   public:
    bool valid() const { return bit_index_ >= 0; }

    Card card() const {
      return Card((Rank)(bit_index_ % 13), (Suit)(bit_index_ / 13));
    }

   private:
    Iter(int bit_index) : bit_index_(bit_index) {
      assert(bit_index >= 0 && bit_index < 52);
    }

    Iter() : bit_index_(-1) {}

    int bit_index_;

    friend class Cards;
  };

  Cards() : bits_(0) {};

  Cards with_card(Card c) const {
    return Cards(bits_ | (((uint64_t)1) << bit_index(c)));
  }

  Iter first() const {
    int k = std::countl_zero(bits_ << 12);
    return k < 64 ? Cards::Iter(51 - k) : Cards::Iter();
  }

  Iter next(Iter i) const {
    assert(i.valid());
    if (i.bit_index_ <= 0) {
      // Beware! Shifting by > 63 bits is undefined behavior (and will not
      // necessarily zero out the value being shifted on some hardware).
      // Therefore, we *must* special case when bit_index_ is 0.
      return Cards::Iter();
    }
    int k = std::countl_zero(bits_ << (64 - i.bit_index_));
    return k < 64 ? Cards::Iter(i.bit_index_ - k - 1) : Cards::Iter();
  }

  bool empty() const { return !bits_; }

  Cards intersect(Cards c) const { return Cards(bits_ & c.bits_); }

  Cards intersect_suit(Suit s) const {
    return Cards(bits_ & (SUIT_MASK << ((uint64_t)s * 13)));
  }

  bool disjoint(Cards c) const { return intersect(c).empty(); }

 private:
  Cards(uint64_t bits) : bits_(bits) { assert(!(bits & INVALID_MASK)); }

  int bit_index(Card c) const { return c.rank() + c.suit() * 13; }

  uint64_t bits_;

  static const uint64_t SUIT_MASK = 0b1111111111111;
  static const uint64_t INVALID_MASK =
      ~0b1111111111111111111111111111111111111111111111111111;
};

class Hand {
 public:
  Hand(Seat lead, Cards w, Cards n, Cards e, Cards s)
      : lead_(lead), w_(w), n_(n), e_(e), s_(s) {
    assert(w.disjoint(n) && w.disjoint(e) && w.disjoint(s));
    assert(n.disjoint(e) & n.disjoint(s));
    assert(e.disjoint(s));
  }

 private:
  Cards w_, n_, e_, s_;
  Seat lead_;
};

std::ostream& operator<<(std::ostream& os, Suit s);
std::ostream& operator<<(std::ostream& os, Rank r);
std::ostream& operator<<(std::ostream& os, Card c);
std::ostream& operator<<(std::ostream& os, Cards c);
std::ostream& operator<<(std::ostream& os, Seat s);
std::ostream& operator<<(std::ostream& os, const Hand& h);
