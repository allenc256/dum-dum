#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string_view>

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
  ACE
};

inline Rank operator++(Rank &r, int) { return (Rank)((int8_t &)r)++; }
inline Rank operator--(Rank &r, int) { return (Rank)((int8_t &)r)--; }

std::istream &operator>>(std::istream &is, Rank &r);
std::ostream &operator<<(std::ostream &os, Rank r);

class Card {
public:
  Card() : index_(0) {}

  Card(int card_index) : index_((uint8_t)card_index) {
    assert(card_index >= 0 && card_index < 52);
  }

  Card(Rank r, Suit s) : index_((uint8_t)((r << 2) | s)) {
    assert(s != NO_TRUMP);
  }

  Card(std::string_view s);
  Card(const char *s);

  Rank rank() const { return (Rank)(index_ >> 2); }
  Suit suit() const { return (Suit)(index_ & 0b11); }

  std::string to_string() const;

  bool operator==(Card other) const { return index_ == other.index_; }

private:
  uint8_t index_;

  friend std::istream &operator>>(std::istream &is, Card &c);
  friend std::ostream &operator<<(std::ostream &os, Card c);
};

class Cards {
public:
  template <bool low_to_high> class Iterator {
  public:
    Card operator*() const { return Card(card_index_); }

    void operator++() {
      assert(card_index_ >= 0);
      if constexpr (low_to_high) {
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

    bool operator==(const Iterator &it) const {
      return card_index_ == it.card_index_;
    }

    static Iterator begin(uint64_t bits) {
      if constexpr (low_to_high) {
        int k = std::countr_zero(bits);
        return k < 64 ? Iterator(bits, k) : end(bits);
      } else {
        int k = std::countl_zero(bits << 12);
        return k < 64 ? Iterator(bits, 51 - k) : end(bits);
      }
    }

    static Iterator end(uint64_t bits) { return Iterator(bits, -1); }

  private:
    Iterator(uint64_t bits, int card_index)
        : bits_(bits),
          card_index_(card_index) {}

    friend class Cards;

    uint64_t bits_;
    int      card_index_;
  };

  template <bool low_to_high> class Iterable {
  public:
    Iterator<low_to_high> begin() const {
      return Iterator<low_to_high>::begin(bits_);
    }

    Iterator<low_to_high> end() const {
      return Iterator<low_to_high>::end(bits_);
    }

  private:
    friend class Cards;

    Iterable(uint64_t bits) : bits_(bits) {}

    uint64_t bits_;
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
  void     add(Card c) { bits_ |= to_card_bit(c); }
  void     add_all(Cards c) { bits_ |= c.bits_; }
  void     remove(Card c) { bits_ &= ~to_card_bit(c); }
  void     remove_all(Cards c) { bits_ &= ~c.bits_; }
  bool     contains(Card c) const { return bits_ & to_card_bit(c); }
  bool     contains_all(Cards c) const { return intersect(c) == c; }
  int      count() const { return std::popcount(bits_); }
  Cards    with(Card c) const { return Cards(bits_ | to_card_bit(c)); }
  Cards    with_all(Cards c) const { return Cards(bits_ | c.bits_); }
  Cards    complement() const { return Cards(~bits_ & ALL_MASK); }
  bool     disjoint(Cards c) const { return intersect(c).empty(); }
  Cards    intersect(Cards c) const { return Cards(bits_ & c.bits_); }
  Cards    without_all(Cards c) const { return Cards(bits_ & ~c.bits_); }

  Cards without_lower(Rank rank) const {
    return Cards(bits_ & (ALL_MASK << (rank * 4)));
  }

  Cards intersect(Suit s) const { return Cards(bits_ & (SUIT_MASK << s)); }

  Cards normalize(uint16_t removed_ranks_mask) const {
    uint64_t b = bits_;
    uint64_t m = 0xfffffffffffffull;
    uint16_t r = (uint16_t)(removed_ranks_mask << 3);

    while (r) {
      int keep = std::countl_zero(r);
      m >>= keep * 4;
      r <<= keep;
      int drop = std::countl_one(r);
      b        = (b & ~m) | (((b & m) << 4) & m);
      r <<= drop;
    }

    return Cards(b);
  }

  Cards normalize(Cards removed) const {
    if (!removed.bits_) {
      return *this;
    }
    assert(disjoint(removed));
    uint64_t bits = bits_;
    for (int i = 1; i < 13; i++) {
      uint64_t keep_new = (0b1111ull << (i * 4)) & removed.bits_;
      keep_new          = keep_new | (keep_new >> 4);
      keep_new          = keep_new | (keep_new >> 8);
      keep_new          = keep_new | (keep_new >> 16);
      keep_new          = keep_new | (keep_new >> 32);
      uint64_t keep_old = ~keep_new;
      bits              = (bits & keep_old) | (((bits << 4)) & keep_new);
    }
    return Cards(bits);
  }

  Cards prune_equivalent(Cards removed) const {
    assert(disjoint(removed));

    constexpr uint64_t init_mask =
        0b1111000000000000000000000000000000000000000000000000ull;

    uint64_t bits      = bits_ & init_mask;
    uint64_t next_mask = init_mask >> 4;
    uint64_t prev      = (init_mask & bits_) >> 4;

    for (int i = 0; i < 12; i++) {
      uint64_t next   = next_mask & bits_;
      uint64_t ignore = next_mask & removed.bits_;
      bits |= ~prev & next;
      next_mask >>= 4;
      prev = (next | (prev & ignore)) >> 4;
    }

    return Cards(bits);
  }

  Iterable<true>  low_to_high() const { return Iterable<true>(bits_); }
  Iterable<false> high_to_low() const { return Iterable<false>(bits_); }

  Card lowest() const {
    assert(!empty());
    return *low_to_high().begin();
  }

  Card highest() const {
    assert(!empty());
    return *high_to_low().begin();
  }

  Card lowest_equivalent(Card card, Cards removed) const {
    uint64_t mask = to_card_bit(card);
    Rank     curr = card.rank();
    Rank     low  = curr;

    while (curr > 0) {
      curr--;
      mask >>= 4;
      bool pres_here      = mask & bits_;
      bool pres_elsewhere = !(mask & removed.bits_);
      if (pres_here) {
        low = curr;
      } else if (pres_elsewhere) {
        break;
      }
    }

    return Card(low, card.suit());
  }

  static Cards all() { return Cards(ALL_MASK); }
  static Cards all(Suit s) { return Cards(SUIT_MASK << s); }

  static Cards higher_ranking(Card card) {
    uint64_t rank_bits = (SUIT_MASK << ((card.rank() + 1) * 4)) & ALL_MASK;
    return Cards(rank_bits << card.suit());
  }

  static Cards higher_ranking_or_eq(Card card) {
    uint64_t rank_bits = (SUIT_MASK << (card.rank() * 4)) & ALL_MASK;
    return Cards(rank_bits << card.suit());
  }

  static Cards lower_ranking(Card card) {
    uint64_t rank_bits = (SUIT_MASK >> ((13 - card.rank()) * 4)) & ALL_MASK;
    return Cards(rank_bits << card.suit());
  }

  std::string to_string() const;

private:
  uint64_t bits_;

  static int      to_card_index(Card c) { return c.rank() * 4 + c.suit(); }
  static uint64_t to_card_bit(Card c) { return to_card_bit(to_card_index(c)); }
  static uint64_t to_card_bit(int card_index) { return 1ull << card_index; }

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

class SuitNormalizer {
public:
  SuitNormalizer() : norm_map_(IDENTITY_MAP), denorm_map_(IDENTITY_MAP) {}

  Rank normalize(Rank rank) const {
    return (Rank)((norm_map_ >> (rank * 4)) & 0b1111);
  }

  Rank denormalize(Rank rank) const {
    return (Rank)((denorm_map_ >> (rank * 4)) & 0b1111);
  }

  void remove(Rank rank) {
    Rank     nr = normalize(rank);
    uint64_t m  = MASK >> ((12 - rank) * 4);
    uint64_t nm = MASK >> ((12 - nr) * 4);
    norm_map_ += ONES & m;
    denorm_map_ = (denorm_map_ & ~nm) | (((denorm_map_ & nm) << 4) & nm);
  }

  void add(Rank rank) {
    uint64_t m = MASK >> ((12 - rank) * 4);
    norm_map_ -= ONES & m;
    Rank     nr = normalize(rank);
    uint64_t nm = MASK >> ((12 - nr) * 4);
    denorm_map_ = (denorm_map_ & ~nm) | ((denorm_map_ & nm) >> 4) |
                  ((uint64_t)rank << (nr * 4));
  }

private:
  static constexpr uint64_t ONES         = 0x0001111111111111ull;
  static constexpr uint64_t IDENTITY_MAP = 0x000cba9876543210ull;
  static constexpr uint64_t MASK         = 0x000fffffffffffffull;

  uint64_t norm_map_;
  uint64_t denorm_map_;
};

class CardNormalizer {
public:
  Card normalize(Card card) const {
    Rank r = norm_[card.suit()].normalize(card.rank());
    return Card(r, card.suit());
  }

  Card denormalize(Card card) const {
    Rank r = norm_[card.suit()].denormalize(card.rank());
    return Card(r, card.suit());
  }

  Cards normalize(Cards cards) const { return cards.normalize(removed_); }

  Cards prune_equivalent(Cards cards) const {
    return cards.prune_equivalent(removed_);
  }

  void remove(Card card) {
    assert(!removed_.contains(card));
    removed_.add(card);
    norm_[card.suit()].remove(card.rank());
  }

  void add(Card card) {
    assert(removed_.contains(card));
    removed_.remove(card);
    norm_[card.suit()].add(card.rank());
  }

  void remove_all(Cards cards) {
    for (Card c : cards.high_to_low()) {
      remove(c);
    }
  }

private:
  Cards          removed_;
  SuitNormalizer norm_[4];
};
