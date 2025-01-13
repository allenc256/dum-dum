#pragma once

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

template <class T> class Slice {
public:
  using value_type = T;

  Slice();
  Slice(std::initializer_list<T> values);
  Slice(Slice &&slice) noexcept;
  Slice(const Slice &)            = delete;
  Slice &operator=(const Slice &) = delete;

  Slice &operator=(Slice &&slice) noexcept;

  const T *begin() const;
  const T *end() const;
  const T &operator[](int i) const;
  T       &operator[](int i);
  int      size() const;

  T   *begin();
  T   *end();
  void remove_at(int i);
  T   &expand();

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
  void check_invariants() const;

private:
  struct Bounds {
    int8_t lower_bound;
    int8_t upper_bound;

    bool operator==(const Bounds &bounds) const = default;

    void tighten(Bounds bounds);
    bool tighter(Bounds bounds) const;
    bool tighter_or_eq(Bounds bounds) const;
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
  TpnBucketKey(Seat next_seat, const Hands &hands);

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

  TpnTable(const Game &game);

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

// ----------------------
// Implementation Details
// ----------------------

template <class T> inline Slice<T>::Slice() : count_(0), capacity_(0) {}

template <class T>
inline Slice<T>::Slice(std::initializer_list<T> values)
    : values_(std::make_unique<T[]>(values.size())),
      count_((int)values.size()),
      capacity_((int)values.size()) {
  int i = 0;
  for (const T &value : values) {
    values_[i++] = value;
  }
}

template <class T> inline Slice<T>::Slice(Slice &&slice) noexcept {
  values_         = std::move(slice.values_);
  count_          = slice.count_;
  capacity_       = slice.capacity_;
  slice.capacity_ = 0;
  slice.count_    = 0;
}

template <class T>
inline Slice<T> &Slice<T>::operator=(Slice &&slice) noexcept {
  values_         = std::move(slice.values_);
  count_          = slice.count_;
  capacity_       = slice.capacity_;
  slice.count_    = 0;
  slice.capacity_ = 0;
  return *this;
}

template <class T> inline T *Slice<T>::begin() { return values_.get(); }
template <class T> T        *Slice<T>::end() { return values_.get() + count_; }
template <class T> const T  *Slice<T>::begin() const { return values_.get(); }
template <class T> T        &Slice<T>::operator[](int i) { return values_[i]; }
template <class T> int       Slice<T>::size() const { return count_; }

template <class T> const T *Slice<T>::end() const {
  return values_.get() + count_;
}

template <class T> const T &Slice<T>::operator[](int i) const {
  return values_[i];
}

template <class T> void Slice<T>::remove_at(int i) {
  assert(i >= 0 && i < count_);
  if (i == count_ - 1) {
    values_[i] = T();
    count_--;
  } else {
    values_[i] = std::move(values_[count_ - 1]);
    count_--;
  }
}

template <class T> T &Slice<T>::expand() {
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