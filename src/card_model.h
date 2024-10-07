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

enum Suit : int8_t {
  CLUBS,
  DIAMONDS,
  HEARTS,
  SPADES,
  NO_TRUMP,
};

constexpr Suit FIRST_SUIT = CLUBS;
constexpr Suit LAST_SUIT  = SPADES;

inline Suit operator++(Suit &s, int) { return (Suit)((int8_t &)s)++; }
inline Suit operator--(Suit &s, int) { return (Suit)((int8_t &)s)--; }

std::istream &operator>>(std::istream &is, Suit &s);
std::ostream &operator<<(std::ostream &os, Suit s);

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
  ACE,
  RANK_UNKNOWN,
};

inline Rank operator++(Rank &r, int) { return (Rank)((int8_t &)r)++; }
inline Rank operator--(Rank &r, int) { return (Rank)((int8_t &)r)--; }

std::istream &operator>>(std::istream &is, Rank &r);
std::ostream &operator<<(std::ostream &os, Rank r);

class Card {
public:
  Card() : rank_(RANK_2), suit_(CLUBS) {}
  Card(Rank r, Suit s) : rank_(r), suit_(s) { assert(s != NO_TRUMP); }
  Card(std::string_view s);

  Rank rank() const { return (Rank)rank_; }
  Suit suit() const { return (Suit)suit_; }
  bool is_rank_unknown() const { return rank_ == RANK_UNKNOWN; }

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

  Cards(std::initializer_list<std::string_view> cards) : bits_(0) {
    for (auto c : cards) {
      add(Card(c));
    }
  }

  uint64_t bits() const { return bits_; }
  bool     empty() const { return !bits_; }
  void     add_all(Cards c) { bits_ |= c.bits_; }
  void     remove_all(Cards c) { bits_ &= ~c.bits_; }
  void     add(int card_index) { bits_ |= to_card_bit(card_index); }
  void     remove(int card_index) { bits_ &= ~to_card_bit(card_index); }
  void     clear() { bits_ = 0; }
  int      count() const { return std::popcount(bits_); }
  Cards    union_with(Cards c) const { return Cards(bits_ | c.bits_); }
  Cards    complement() const { return Cards(~bits_ & ALL_MASK); }
  bool     disjoint(Cards c) const { return intersect(c).empty(); }
  Cards    intersect(Cards c) const { return Cards(bits_ & c.bits_); }
  Cards    subtract(Cards c) const { return Cards(bits_ & ~c.bits_); }
  Cards    honors() const { return Cards(bits_ & HONORS_MASK); }

  void add(Card c) {
    assert(!c.is_rank_unknown());
    bits_ |= to_card_bit(c);
  }

  void remove(Card c) {
    assert(!c.is_rank_unknown());
    bits_ &= ~to_card_bit(c);
  }

  bool contains(Card c) const {
    assert(!c.is_rank_unknown());
    return bits_ & to_card_bit(c);
  }

  Cards with(Card c) const {
    assert(!c.is_rank_unknown());
    return Cards(bits_ | to_card_bit(c));
  }

  Cards intersect(Suit s) const { return Cards(bits_ & (SUIT_MASK << s)); }

  int top_ranks(Suit s) const {
    uint64_t bit   = 1ull << (48 + s);
    int      count = 0;
    while (bits_ & bit) {
      bit >>= 4;
      count++;
    }
    return count;
  }

  Cards collapse(Cards ignorable) const {
    if (!ignorable.bits_) {
      return *this;
    }
    assert(disjoint(ignorable));
    uint64_t bits = bits_;
    for (int i = 1; i < 13; i++) {
      uint64_t keep_new = (0b1111ull << (i * 4)) & ignorable.bits_;
      keep_new          = keep_new | (keep_new >> 4);
      keep_new          = keep_new | (keep_new >> 8);
      keep_new          = keep_new | (keep_new >> 16);
      keep_new          = keep_new | (keep_new >> 32);
      uint64_t keep_old = ~keep_new;
      bits              = (bits & keep_old) | (((bits << 4)) & keep_new);
    }
    return Cards(bits);
  }

  Cards prune_equivalent(Cards ignorable) const {
    assert(disjoint(ignorable));

    constexpr uint64_t init_mask =
        0b1111000000000000000000000000000000000000000000000000ull;

    uint64_t bits      = bits_ & init_mask;
    uint64_t next_mask = init_mask >> 4;
    uint64_t prev      = (init_mask & bits_) >> 4;

    for (int i = 0; i < 12; i++) {
      uint64_t next   = next_mask & bits_;
      uint64_t ignore = next_mask & ignorable.bits_;
      bits |= ~prev & next;
      next_mask >>= 4;
      prev = (next | (prev & ignore)) >> 4;
    }

    return Cards(bits);
  }

  Iter iter_highest() const {
    int k = std::countl_zero(bits_ << 12);
    return k < 64 ? Cards::Iter(51 - k) : Cards::Iter();
  }

  Iter iter_lower(Iter i) const {
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

  Iter iter_lowest() const {
    int k = std::countr_zero(bits_);
    return k < 64 ? Cards::Iter(k) : Cards::Iter();
  }

  Iter iter_higher(Iter i) const {
    assert(i.valid());
    int k = std::countr_zero(bits_ >> (i.card_index_ + 1));
    return k < 64 ? Cards::Iter(i.card_index_ + k + 1) : Cards::Iter();
  }

  static Cards all() { return Cards(ALL_MASK); }
  static Cards all(Suit s) { return Cards(SUIT_MASK << s); }

  static Cards higher_ranking(Card card) {
    assert(!card.is_rank_unknown());
    uint64_t rank_bits = (SUIT_MASK << ((card.rank() + 1) * 4)) & ALL_MASK;
    return Cards(rank_bits << card.suit());
  }

  static Cards lower_ranking(Card card) {
    assert(!card.is_rank_unknown());
    uint64_t rank_bits = (SUIT_MASK >> ((13 - card.rank()) * 4)) & ALL_MASK;
    return Cards(rank_bits << card.suit());
  }

private:
  uint64_t bits_;

  static int      to_card_index(Card c) { return c.rank() * 4 + c.suit(); }
  static uint64_t to_card_bit(Card c) { return to_card_bit(to_card_index(c)); }
  static uint64_t to_card_bit(int card_index) { return 1ull << card_index; }

  static Card from_card_index(int card_index) {
    return Card((Rank)(card_index / 4), (Suit)(card_index % 4));
  }

  static const uint64_t HONORS_MASK =
      0b1111111111111111000000000000000000000000000000000000ull;
  static const uint64_t SUIT_MASK =
      0b0001000100010001000100010001000100010001000100010001ull;
  static const uint64_t ALL_MASK =
      0b1111111111111111111111111111111111111111111111111111ull;

  friend bool operator==(Cards c1, Cards c2);

  template <typename H> friend H AbslHashValue(H h, const Cards &c) {
    return H::combine(std::move(h), c.bits_);
  }
};

std::istream &operator>>(std::istream &is, Cards &c);
std::ostream &operator<<(std::ostream &os, Cards c);

inline bool operator==(Cards c1, Cards c2) { return c1.bits_ == c2.bits_; }