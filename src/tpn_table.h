#pragma once

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

template <class T> class Slice {
public:
  using value_type = T;

  Slice() : count_(0), capacity_(0) {}

  Slice(std::initializer_list<T> values)
      : values_(std::make_unique<T[]>(values.size())),
        count_((int)values.size()),
        capacity_((int)values.size()) {
    int i = 0;
    for (const T &value : values) {
      values_[i++] = value;
    }
  }

  Slice(const Slice &)            = delete;
  Slice &operator=(const Slice &) = delete;

  Slice(Slice &&slice) noexcept {
    values_         = std::move(slice.values_);
    count_          = slice.count_;
    capacity_       = slice.capacity_;
    slice.capacity_ = 0;
    slice.count_    = 0;
  }

  Slice &operator=(Slice &&slice) noexcept {
    values_         = std::move(slice.values_);
    count_          = slice.count_;
    capacity_       = slice.capacity_;
    slice.count_    = 0;
    slice.capacity_ = 0;
    return *this;
  }

  T       *begin() { return values_.get(); }
  T       *end() { return values_.get() + count_; }
  const T *begin() const { return values_.get(); }
  const T *end() const { return values_.get() + count_; }
  const T &operator[](int i) const { return values_[i]; }
  T       &operator[](int i) { return values_[i]; }
  int      size() const { return count_; }

  void remove_at(int i) {
    assert(i >= 0 && i < count_);
    if (i == count_ - 1) {
      values_[i] = T();
      count_--;
    } else {
      values_[i] = std::move(values_[count_ - 1]);
      count_--;
    }
  }

  T &expand() {
    if (count_ == capacity_) {
      if (capacity_ == 0) {
        capacity_ = 4;
      } else {
        capacity_ *= 2;
      }

      std::unique_ptr<T[]> tmp_ = std::move(values_);
      values_                   = std::make_unique<T[]>(capacity_);
      for (int i = 0; i < count_; i++) {
        values_[i] = std::move(tmp_[i]);
      }
    }

    T &result = values_[count_++];
    result    = T();
    return result;
  }

private:
  std::unique_ptr<T[]> values_;
  int                  count_;
  int                  capacity_;
};

class TpnBucket {
public:
  static constexpr int MIN_BOUND = 0;
  static constexpr int MAX_BOUND = 13;

  struct Stats {
    int64_t entries       = 0;
    int64_t lookup_hits   = 0;
    int64_t lookup_misses = 0;
    int64_t lookup_reads  = 0;
    int64_t insert_hits   = 0;
    int64_t insert_misses = 0;
    int64_t insert_reads  = 0;
  };

  const Stats &stats() const { return stats_; }

  bool lookup(
      const Hands &hands,
      int          alpha,
      int          beta,
      int         &score,
      Cards       &winners_by_rank
  ) const;

  void insert(const Hands &partition, int lower_bound, int upper_bound);

  void check_invariants() const {
    for (const Entry &entry : entries_) {
      check_invariants(entry);
    }
  }

private:
  struct Bounds {
    int8_t lower_bound;
    int8_t upper_bound;

    bool operator==(const Bounds &bounds) const = default;

    void tighten(Bounds bounds) {
      assert(bounds.lower_bound <= upper_bound);
      assert(bounds.upper_bound >= lower_bound);
      lower_bound = std::max(lower_bound, bounds.lower_bound);
      upper_bound = std::min(upper_bound, bounds.upper_bound);
    }

    bool tighter(Bounds bounds) const {
      return tighter_or_eq(bounds) && *this != bounds;
    }

    bool tighter_or_eq(Bounds bounds) const {
      return lower_bound >= bounds.lower_bound &&
             upper_bound <= bounds.upper_bound;
    }
  };

  struct Entry {
    Hands        partition;
    Bounds       bounds;
    Slice<Entry> children;
  };

  bool lookup(
      const Slice<Entry> &slice,
      const Hands        &hands,
      int                 alpha,
      int                 beta,
      int                &score,
      Cards              &winners_by_rank
  ) const;

  void insert(Slice<Entry> &slice, const Hands &partition, Bounds bounds);
  void transfer_generalized(Slice<Entry> &src, Entry &dest) const;
  void tighten_child_bounds(Entry &entry);
  void check_invariants(const Entry &entry) const;

  Slice<Entry>  entries_;
  mutable Stats stats_;
};

class TpnBucketKey {
public:
  TpnBucketKey(Seat next_seat, const Hands &hands) : bits_(next_seat) {
    for (Seat seat = LAST_SEAT; seat >= FIRST_SEAT; seat--) {
      Cards hand = hands.hand(seat);
      for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
        bits_ = (bits_ << 3) | hand.intersect(suit).count();
      }
    }
  }

  template <typename H> friend H AbslHashValue(H h, const TpnBucketKey &k) {
    return H::combine(std::move(h), k.bits_);
  }

  bool operator==(const TpnBucketKey &other) const = default;

  friend std::ostream &operator<<(std::ostream &os, const TpnBucketKey &key);

private:
  uint64_t bits_;
};

class TpnTable {
public:
  struct Stats {
    int64_t buckets       = 0;
    int64_t entries       = 0;
    int64_t lookup_hits   = 0;
    int64_t lookup_misses = 0;
    int64_t lookup_reads  = 0;
    int64_t insert_hits   = 0;
    int64_t insert_misses = 0;
    int64_t insert_reads  = 0;
  };

  TpnTable(const Game &game)
      : game_(game),
        lookup_misses_(0),
        insert_misses_(0) {}

  bool  lookup(int alpha, int beta, int &score, Cards &winners_by_rank) const;
  void  insert(Cards winners_by_rank, int lower_bound, int upper_bound);
  Stats stats() const;
  void  check_invariants() const;

private:
  using HashTable = absl::flat_hash_map<TpnBucketKey, TpnBucket>;

  const Game &game_;
  HashTable   table_;
  int64_t     lookup_misses_;
  int64_t     insert_misses_;
};
