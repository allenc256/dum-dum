#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <ostream>

enum Suit : uint8_t { C, D, H, S };
enum Rank : uint8_t { R2, R3, R4, R5, R6, R7, R8, R9, T, J, Q, K, A };

class Card {
 public:
  Card(Rank r, Suit s);
  Card(std::string_view utf8_str);

  Suit suit() const;
  Rank rank() const;

 private:
  Rank rank_;
  Suit suit_;

  friend class Cards;
};

class Cards {
 public:
  class Iter {
   public:
    bool valid() const;
    Card card() const;

   private:
    Iter(int index);
    Iter();

    int index_;

    friend class Cards;
  };

  Cards();

  Cards with_card(Card c) const;

  Cards of_suit(Suit s) const;

  Iter first() const;
  Iter next(Iter i) const;

 private:
  Cards(uint64_t bits);

  int to_index(Card c) const;

  uint64_t bits_;

  static const uint64_t SUIT_MASK = 0b1111111111111;
  static const uint64_t INVALID_MASK =
      ~0b1111111111111111111111111111111111111111111111111111;
};

std::ostream& operator<<(std::ostream& os, const Suit& s);
std::ostream& operator<<(std::ostream& os, const Rank& r);
std::ostream& operator<<(std::ostream& os, const Card& c);
std::ostream& operator<<(std::ostream& os, const Cards& c);

inline Card::Card(Rank r, Suit s) : rank_(r), suit_(s) {}

inline Suit Card::suit() const { return suit_; }

inline Rank Card::rank() const { return rank_; }

inline Cards::Cards() : bits_(0) {}

inline Cards::Cards(uint64_t cards) : bits_(cards) {
  assert(!(cards & INVALID_MASK));
}

inline Cards Cards::with_card(Card c) const {
  return Cards(bits_ | (((uint64_t)1) << to_index(c)));
}

inline int Cards::to_index(Card c) const { return c.rank() + c.suit() * 13; }

inline Cards::Iter::Iter() : index_(-1) {}

inline Cards::Iter::Iter(int index) : index_(index) {
  assert(index >= 0 && index < 52);
}

inline bool Cards::Iter::valid() const { return index_ >= 0; };

inline Card Cards::Iter::card() const {
  return Card((Rank)(index_ % 13), (Suit)(index_ / 13));
}

inline Cards::Iter Cards::first() const {
  int k = std::countl_zero(bits_ << 12);
  return k < 64 ? Cards::Iter(51 - k) : Cards::Iter();
}

inline Cards::Iter Cards::next(Cards::Iter it) const {
  assert(it.valid());
  if (it.index_ <= 0) {
    // N.B., special case 0 here since shift by > 63 bits results
    // in undefined behavior in C++ below!
    return Cards::Iter();
  }
  int k = std::countl_zero(bits_ << (64 - it.index_));
  return k < 64 ? Cards::Iter(it.index_ - k - 1) : Cards::Iter();
}

inline Cards Cards::of_suit(Suit s) const {
  return Cards(bits_ & (SUIT_MASK << ((uint64_t)s * 13)));
}
