#pragma once

#include "game_model.h"
#include "small_ints.h"

#include <absl/container/flat_hash_map.h>

class TpnTableValue {
public:
  TpnTableValue() : TpnTableValue(0, 0, 0, std::nullopt) {}

  TpnTableValue(int lower_bound, int upper_bound, int max_depth)
      : TpnTableValue(lower_bound, upper_bound, max_depth, std::nullopt) {}

  TpnTableValue(
      int                 lower_bound,
      int                 upper_bound,
      int                 max_depth,
      std::optional<Card> pv_play
  )
      : lower_bound_(to_int8_t(lower_bound)),
        upper_bound_(to_int8_t(upper_bound)),
        max_depth_(to_int8_t(max_depth)),
        pv_play_(pv_play) {
    assert(lower_bound >= -1 && lower_bound <= upper_bound);
    assert(upper_bound >= 0 && upper_bound <= 14);
    assert(max_depth >= 0 && max_depth <= 13);
    assert(!pv_play.has_value() || lower_bound == upper_bound);
  }

  int  lower_bound() const { return lower_bound_; }
  int  upper_bound() const { return upper_bound_; }
  int  max_depth() const { return max_depth_; }
  bool has_tight_bounds() const { return lower_bound_ == upper_bound_; }
  std::optional<Card> pv_play() const { return pv_play_; }

  bool update_lower_bound(int lb) {
    assert(lb <= upper_bound_);
    if (lb > lower_bound_) {
      lower_bound_ = to_int8_t(lb);
      return true;
    } else {
      return false;
    }
  }

  bool update_upper_bound(int ub) {
    assert(ub >= lower_bound_);
    if (ub < upper_bound_) {
      upper_bound_ = to_int8_t(ub);
      return true;
    } else {
      return false;
    }
  }

  void update_pv_play(Card pv_play) {
    assert(!pv_play_.has_value());
    assert(has_tight_bounds());
    pv_play_ = pv_play;
  }

private:
  int8_t              lower_bound_;
  int8_t              upper_bound_;
  int8_t              max_depth_;
  std::optional<Card> pv_play_;
};

class TpnTable {
public:
  TpnTable(Game &game) : game_(game) {}

  struct LookupResult {
    bool          found;
    TpnTableValue value;
  };

  LookupResult lookup_value(int max_depth) const {
    auto it = table_.find(game_.normalized_key());
    if (it != table_.end() && it->second.max_depth() >= max_depth) {
      const TpnTableValue &norm_value = it->second;
      int lb = norm_value.lower_bound() + game_.tricks_taken_by_ns();
      int ub = norm_value.upper_bound() + game_.tricks_taken_by_ns();
      std::optional<Card> pv_play = denormalize_card(norm_value.pv_play());
      return {
          .found = true,
          .value = TpnTableValue(lb, ub, max_depth, pv_play),
      };
    } else {
      int lb = game_.tricks_taken_by_ns();
      int ub = game_.tricks_taken_by_ns() + game_.tricks_left();
      return {
          .found = false,
          .value = TpnTableValue(lb, ub, max_depth, std::nullopt)
      };
    }
  }

  void update_value(TpnTableValue value) {
    assert(value.lower_bound() >= game_.tricks_taken_by_ns());
    assert(value.upper_bound() >= game_.tricks_taken_by_ns());
    int lb = value.lower_bound() - game_.tricks_taken_by_ns();
    int ub = value.upper_bound() - game_.tricks_taken_by_ns();
    assert(lb <= game_.tricks_left());
    assert(ub <= game_.tricks_left());
    std::optional<Card> pv_play = normalize_card(value.pv_play());
    table_[game_.normalized_key()] =
        TpnTableValue(lb, ub, value.max_depth(), pv_play);
  }

  size_t size() const { return table_.size(); }

private:
  std::optional<Card> denormalize_card(const std::optional<Card> &card) const {
    if (card.has_value()) {
      return game_.denormalize_card(*card);
    } else {
      return std::nullopt;
    }
  }

  std::optional<Card> normalize_card(const std::optional<Card> &card) const {
    if (card.has_value()) {
      return game_.normalize_card(*card);
    } else {
      return std::nullopt;
    }
  }

  Game                                       &game_;
  absl::flat_hash_map<GameKey, TpnTableValue> table_;
};

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

  // void normalize(Game &game) {
  //   for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
  //     rank_cutoffs_[suit] =
  //         game.normalize_rank_cutoff(rank_cutoffs_[suit], suit);
  //   }
  // }

  // void denormalize(Game &game) {
  //   for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
  //     rank_cutoffs_[suit] =
  //         game.denormalize_rank_cutoff(rank_cutoffs_[suit], suit);
  //   }
  // }

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
  AbsSuitState(Rank rank_cutoff, Suit suit, Game &game)
      : AbsSuitState(rank_cutoff, suit, game.hands()) {}

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

  bool matches(Game &game, Suit suit) const {
    AbsSuitState state(rank_cutoff(), suit, game);
    return bits_ == state.bits_;
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
  AbsState(AbsLevel level, Game &game)
      : suit_states_{
            make_suit_state(level, game, 0),
            make_suit_state(level, game, 1),
            make_suit_state(level, game, 2),
            make_suit_state(level, game, 3),
        }, lead_seat_(game.next_seat()) {
    assert(game.start_of_trick());
  }

  bool matches(Game &game) const {
    assert(game.start_of_trick());
    if (game.next_seat() != lead_seat_) {
      return false;
    }
    for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
      if (!suit_states_[suit].matches(game, suit)) {
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
  static AbsSuitState make_suit_state(AbsLevel level, Game &game, int index) {
    return AbsSuitState(level.rank_cutoff((Suit)index), (Suit)index, game);
  }

  AbsSuitState suit_states_[4];
  Seat         lead_seat_;
};

// class TpnPlay {
// public:
//   enum Type : int8_t { UNKNOWN, HIGH_CARD, LOW_CARD };

//   TpnPlay() : type_(UNKNOWN) {}

//   TpnPlay(const AbsLevel &level, Card card) {
//     Rank rank_cutoff = level.rank_cutoff(card.suit());
//     if (card.rank() <= rank_cutoff) {
//       type_ = LOW_CARD;
//     } else {
//       type_      = HIGH_CARD;
//       high_card_ = card;
//     }
//   }

//   Type type() const { return type_; }

//   Card high_card() const {
//     assert(type_ == HIGH_CARD);
//     return high_card_;
//   }

// private:
//   Type type_;
//   Card high_card_;
// };

class TpnTable2 {
public:
  struct Value {
    AbsLevel            level;
    int8_t              lower_bound;
    int8_t              upper_bound;
    std::optional<Card> pv_play;
  };

  TpnTable2(Game &game) : game_(game) {}

  bool   lookup_value(int alpha, int beta, int max_depth, Value &value);
  void   upsert_value(int max_depth, const Value &value);
  size_t size() const { return multimap_.size(); }

private:
  enum PlayType { UNKNOWN, LOW_CARD, HIGH_CARD };

  struct Entry {
    AbsState state;
    int8_t   lower_bound;
    int8_t   upper_bound;
    int8_t   max_depth;
    PlayType pv_play_type;
    Card     pv_play_card;

    Entry(
        const AbsState &state,
        int             lower_bound,
        int             upper_bound,
        int             max_depth,
        PlayType        pv_play_type,
        Card            pv_play_card
    )
        : state(state),
          lower_bound(to_int8_t(lower_bound)),
          upper_bound(to_int8_t(upper_bound)),
          max_depth(to_int8_t(max_depth)),
          pv_play_type(pv_play_type),
          pv_play_card(pv_play_card) {}
  };

  using Multimap =
      std::unordered_multimap<SeatShapes, Entry, absl::Hash<SeatShapes>>;

  bool
  lookup_value_normed(int alpha, int beta, int max_depth, Value &value) const;
  void upsert_value_normed(int max_depth, const Value &value);

  void norm_value(Value &value);
  void denorm_value(Value &value);

  Card choose_pv_play(const Entry &entry) const;

  Game    &game_;
  Multimap multimap_;
};
