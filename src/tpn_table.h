#pragma once

#include "game_model.h"
#include "small_ints.h"

#include <absl/container/flat_hash_map.h>

class SeatShape {
public:
  SeatShape() : bits_(0) {}

  SeatShape(Cards hand) {
    bits_ = 0;
    for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
      bits_ |= hand.intersect(suit).count() << (suit * 4);
    }
  }

  int card_count(Suit suit) const { return (bits_ >> (suit * 4)) & 0xf; }

  template <typename H> friend H AbslHashValue(H h, SeatShape s) {
    return H::combine(std::move(h), s.bits_);
  }

  bool operator==(const SeatShape &other) const = default;

private:
  friend struct std::hash<SeatShape>;

  uint16_t bits_;
};

class SeatShapes {
public:
  SeatShapes(const Game &game) {
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      seat_shapes_[seat] = SeatShape(game.hand(seat));
    }
  }

  template <typename H> friend H AbslHashValue(H h, const SeatShapes &s) {
    return H::combine(std::move(h), s.seat_shapes_);
  }

  bool operator==(const SeatShapes &other) const = default;

private:
  std::array<SeatShape, 4> seat_shapes_;
};

class AbsLevel {
public:
  AbsLevel() : rank_cutoffs_{ACE, ACE, ACE, ACE} {}

  AbsLevel(Rank west, Rank north, Rank east, Rank south)
      : rank_cutoffs_{west, north, east, south} {}

  AbsLevel(const Hands &hands, const Trick &trick);

  Rank rank_cutoff(Suit suit) const { return rank_cutoffs_[suit]; }

  bool is_high_card(Card card) const {
    return card.rank() > rank_cutoff(card.suit());
  }

  void intersect(const AbsLevel &other) {
    for (int i = 0; i < 4; i++) {
      if (other.rank_cutoffs_[i] < rank_cutoffs_[i]) {
        rank_cutoffs_[i] = other.rank_cutoffs_[i];
      }
    }
  }

  void normalize(Game &game);
  void denormalize(Game &game);

  template <typename H> friend H AbslHashValue(H h, const AbsLevel &l) {
    return H::combine(std::move(h), l.rank_cutoffs_);
  }

  bool operator==(const AbsLevel &other) const = default;

private:
  std::array<Rank, 4> rank_cutoffs_;
};

std::ostream &operator<<(std::ostream &os, const AbsLevel &level);

class AbsSuitState {
public:
  AbsSuitState(Rank rank_cutoff, Suit suit, const Hands &hands) {
    bits_ = (uint64_t)rank_cutoff << 60;

    constexpr uint64_t mask_all = 0x1111111111111ull;
    uint64_t           b0       = (hands.hand(WEST).bits() >> suit) & mask_all;
    uint64_t           b1       = (hands.hand(NORTH).bits() >> suit) & mask_all;
    uint64_t           b2       = (hands.hand(EAST).bits() >> suit) & mask_all;
    uint64_t           b3       = (hands.hand(SOUTH).bits() >> suit) & mask_all;
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

  bool operator==(const AbsSuitState &s) const = default;

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
  AbsState(AbsLevel normed_level, Game &game)
      : suit_states_{
            make_suit_state(normed_level, game, 0),
            make_suit_state(normed_level, game, 1),
            make_suit_state(normed_level, game, 2),
            make_suit_state(normed_level, game, 3),
        }, lead_seat_(game.next_seat()) {
    assert(game.start_of_trick());
  }

  bool matches(Game &game) {
    assert(game.start_of_trick());
    if (game.next_seat() != lead_seat_) {
      return false;
    }
    for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
      if (!matches(suit_states_[suit], game, suit)) {
        return false;
      }
    }
    return true;
  }

  bool operator==(const AbsState &s) const = default;

  AbsLevel level() const {
    return AbsLevel(
        suit_states_[0].rank_cutoff(),
        suit_states_[1].rank_cutoff(),
        suit_states_[2].rank_cutoff(),
        suit_states_[3].rank_cutoff()
    );
  }

  friend std::ostream &operator<<(std::ostream &os, const AbsState &s);

private:
  bool matches(const AbsSuitState &suit_state, Game &game, Suit suit) {
    AbsSuitState s =
        AbsSuitState(suit_state.rank_cutoff(), suit, game.normalized_hands());
    return suit_state == s;
  }

  static AbsSuitState
  make_suit_state(AbsLevel normed_level, Game &game, int index) {
    return AbsSuitState(
        normed_level.rank_cutoff((Suit)index),
        (Suit)index,
        game.normalized_hands()
    );
  }

  AbsSuitState suit_states_[4];
  Seat         lead_seat_;
};

class TpnTable {
public:
  struct Value {
    AbsLevel            level;
    int8_t              lower_bound;
    int8_t              upper_bound;
    std::optional<Card> pv_play;
  };

  TpnTable(Game &game) : game_(game) {}

  bool   lookup_value(int alpha, int beta, int max_depth, Value &value);
  void   upsert_value(int max_depth, const Value &value);
  size_t size() const { return multimap_.size(); }

private:
  struct Entry {
    AbsState            state;
    int8_t              lower_bound;
    int8_t              upper_bound;
    int8_t              max_depth;
    std::optional<Card> pv_play;

    Entry(
        const AbsState     &state,
        int                 lower_bound,
        int                 upper_bound,
        int                 max_depth,
        std::optional<Card> pv_play
    )
        : state(state),
          lower_bound(to_int8_t(lower_bound)),
          upper_bound(to_int8_t(upper_bound)),
          max_depth(to_int8_t(max_depth)),
          pv_play(pv_play) {}
  };

  using Multimap =
      std::unordered_multimap<SeatShapes, Entry, absl::Hash<SeatShapes>>;

  bool lookup_value_normed(int alpha, int beta, int max_depth, Value &value);
  void upsert_value_normed(int max_depth, const Value &value);

  void norm_value(Value &value);
  void denorm_value(Value &value);

  Game    &game_;
  Multimap multimap_;
};
