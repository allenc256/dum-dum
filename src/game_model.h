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

class GameKey {
public:
  GameKey(Cards west, Cards north, Cards east, Cards south, Seat next_seat)
      : west_(west),
        north_(north),
        east_(east),
        south_(south),
        next_seat_(next_seat) {}

  template <typename H> friend H AbslHashValue(H h, const GameKey &k) {
    return H::combine(
        std::move(h), k.west_, k.north_, k.east_, k.south_, k.next_seat_
    );
  }

  friend bool operator==(const GameKey &lhs, const GameKey &rhs) {
    return lhs.west_ == rhs.west_ && lhs.north_ == rhs.north_ &&
           lhs.east_ == rhs.east_ && lhs.south_ == rhs.south_ &&
           lhs.next_seat_ == rhs.next_seat_;
  }

private:
  Cards west_;
  Cards north_;
  Cards east_;
  Cards south_;
  Seat  next_seat_;
};

class AbsLevel {
public:
  AbsLevel(std::array<Rank, 4> rank_cutoffs) : rank_cutoffs_(rank_cutoffs) {}

  Rank rank_cutoff(Suit suit) const { return rank_cutoffs_[suit]; }

  bool operator==(const AbsLevel &other) const {
    return rank_cutoffs_ == other.rank_cutoffs_;
  }

  template <typename H> friend H AbslHashValue(H h, const AbsLevel &l) {
    return H::combine(std::move(h), l.rank_cutoffs_);
  }

private:
  std::array<Rank, 4> rank_cutoffs_;
};

class AbsSuitState {
public:
  AbsSuitState() : bits_(0) {}

  void init(Rank rank_cutoff, Suit suit, const Cards hands[4]) {
    bits_ = (uint64_t)rank_cutoff << 60;

    constexpr uint64_t mask_all  = 0x1111111111111ull;
    uint64_t           b0        = (hands[0].bits() >> suit) & mask_all;
    uint64_t           b1        = (hands[1].bits() >> suit) & mask_all;
    uint64_t           b2        = (hands[2].bits() >> suit) & mask_all;
    uint64_t           b3        = (hands[3].bits() >> suit) & mask_all;
    uint64_t           mask_high = mask_all << ((rank_cutoff + 1) * 4);
    uint64_t           mask_low  = ~mask_high & mask_all;
    bits_ |= (b0 & mask_high) << 8;
    bits_ |= (b1 & mask_high) << 9;
    bits_ |= (b2 & mask_high) << 10;
    bits_ |= (b3 & mask_high) << 11;

    if (rank_cutoff == 0) {
      bits_ |= std::popcount(b0 & mask_low);
      bits_ |= std::popcount(b1 & mask_low) << 1;
      bits_ |= std::popcount(b2 & mask_low) << 2;
      bits_ |= std::popcount(b3 & mask_low) << 3;
    } else if (rank_cutoff >= 1) {
      bits_ |= std::popcount(b0 & mask_low);
      bits_ |= std::popcount(b1 & mask_low) << 4;
      bits_ |= std::popcount(b2 & mask_low) << 8;
      bits_ |= std::popcount(b3 & mask_low) << 12;
    }
  }

  Rank rank_cutoff() const { return (Rank)(bits_ >> 60); }

  Cards high_cards(Seat seat, Suit suit) const {
    Rank               r         = rank_cutoff();
    constexpr uint64_t mask_all  = 0x1111111111111ull;
    uint64_t           mask_high = (mask_all << ((r + 1) * 4)) & mask_all;
    uint64_t           b         = ((bits_ >> (seat + 8)) & mask_high) << suit;
    return Cards(b);
  }

  int low_cards(Seat seat) const {
    Rank r = rank_cutoff();
    if (r == 0) {
      return (bits_ >> seat) & 0x1;
    } else {
      return (bits_ >> (seat * 4)) & 0xf;
    }
  }

private:
  // Layout of bits_
  // ---------------
  //
  // CASE 0: rank_cutoff == 0:
  //
  //    bits_: 0xrcccccccccccc00n
  //
  //    r: rank_cutoff (1 nibble)
  //    c: high card bits (12 nibbles)
  //    n: low card bits (1 nibble)
  //
  // CASE 1: rank_cutoff >= 1:
  //
  //    bits_: 0xrcccccccccccnnnn
  //
  //    r: rank_cutoff (1 nibble)
  //    c: high card bits (0 to 11 nibbles)
  //    n: low card counts (1 nibble per seat, 4 nibbles total)
  //
  uint64_t bits_;
};

class AbsState {
public:
  AbsSuitState &suit_state(Seat seat) { return suit_states_[seat]; }

private:
  AbsSuitState suit_states_[4];
};

class Game {
public:
  Game(Suit trump_suit, Seat first_lead_seat, Cards hands[4]);

  Suit  trump_suit() const { return trump_suit_; }
  Seat  lead_seat() const { return lead_seat_; }
  Cards hand(Seat seat) const { return hands_[seat]; }
  Seat  next_seat() const { return next_seat_; }
  Seat  next_seat(int i) const { return right_seat(next_seat_, i); }

  Trick       &current_trick() { return tricks_[tricks_taken_]; }
  const Trick &current_trick() const { return tricks_[tricks_taken_]; }

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

  const GameKey &normalized_key() {
    assert(start_of_trick());
    auto &cached = norm_key_stack_[tricks_taken_];
    if (!cached.has_value()) {
      cached.emplace(
          card_normalizer_.normalize(hands_[WEST]),
          card_normalizer_.normalize(hands_[NORTH]),
          card_normalizer_.normalize(hands_[EAST]),
          card_normalizer_.normalize(hands_[SOUTH]),
          next_seat_
      );
    }
    return *cached;
  }

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

  Cards                  hands_[4];
  Suit                   trump_suit_;
  Seat                   lead_seat_;
  Seat                   next_seat_;
  Trick                  tricks_[14];
  int                    tricks_taken_;
  int                    tricks_max_;
  int                    tricks_taken_by_ns_;
  CardNormalizer         card_normalizer_;
  std::optional<GameKey> norm_key_stack_[14];

  friend std::ostream &operator<<(std::ostream &os, const Game &g);
};
