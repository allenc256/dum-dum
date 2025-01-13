#pragma once

#include <vector>

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

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
    Hands              partition;
    Bounds             bounds;
    std::vector<Entry> children;
  };

  bool lookup(
      const std::vector<Entry> &slice,
      const Hands              &hands,
      int                       alpha,
      int                       beta,
      int                      &score,
      Cards                    &winners_by_rank
  ) const;

  void insert(std::vector<Entry> &slice, const Hands &partition, Bounds bounds);
  void transfer_generalized(std::vector<Entry> &src, Entry &dest) const;
  void tighten_child_bounds(Entry &entry);
  void check_invariants(const Entry &entry) const;

  static void remove_at(std::vector<Entry> &slice, std::size_t index);

  std::vector<Entry> entries_;
  mutable Stats      stats_;
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
