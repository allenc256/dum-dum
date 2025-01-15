#include "tpn_table.h"

static bool generalizes(const Hands &partition1, const Hands &partition2) {
  return partition2.contains_all(partition1);
}

bool TpnBucket::lookup(
    const Hands &hands, int alpha, int beta, int &score, Cards &winners_by_rank
) const {
  bool success = lookup(entries_, hands, alpha, beta, score, winners_by_rank);
  if (success) {
    stats_.lookup_hits++;
  } else {
    stats_.lookup_misses++;
  }
  return success;
}

void TpnBucket::insert(
    const Hands &partition, int lower_bound, int upper_bound
) {
  assert(lower_bound <= upper_bound);
  assert(lower_bound >= MIN_BOUND && upper_bound <= MAX_BOUND);
  Bounds bounds = {
      .lower_bound = (int8_t)lower_bound, .upper_bound = (int8_t)upper_bound
  };
  insert(entries_, partition, bounds);
}

bool TpnBucket::lookup(
    const std::vector<Entry> &entries,
    const Hands              &hands,
    int                       alpha,
    int                       beta,
    int                      &score,
    Cards                    &winners_by_rank
) const {
  for (auto &entry : entries) {
    stats_.lookup_reads++;
    if (hands.contains_all(entry.partition)) {
      if (entry.bounds.lower_bound == entry.bounds.upper_bound ||
          entry.bounds.lower_bound >= beta) {
        score           = entry.bounds.lower_bound;
        winners_by_rank = entry.partition.all_cards();
        return true;
      }
      if (entry.bounds.upper_bound <= alpha) {
        score           = entry.bounds.upper_bound;
        winners_by_rank = entry.partition.all_cards();
        return true;
      }
    }
    if (lookup(entry.children, hands, alpha, beta, score, winners_by_rank)) {
      return true;
    }
  }
  return false;
}

void TpnBucket::transfer_generalized(std::vector<Entry> &src, Entry &dest)
    const {
  for (std::size_t i = 0; i < src.size(); i++) {
    auto &e = src[i];
    if (generalizes(dest.partition, e.partition)) {
      dest.children.emplace_back(std::move(e));
      remove_at(src, i);
      i--;
    }
  }
}

void TpnBucket::insert(
    std::vector<Entry> &entries, const Hands &partition, Bounds bounds
) {
  for (auto &entry : entries) {
    stats_.insert_reads++;
    if (partition == entry.partition) {
      if (!entry.bounds.tighter_or_eq(bounds)) {
        entry.bounds.tighten(bounds);
        tighten_child_bounds(entry);
      }
      stats_.insert_hits++;
      return;
    } else if (generalizes(entry.partition, partition)) {
      if (entry.bounds.tighter_or_eq(bounds)) {
        stats_.insert_hits++;
        return;
      } else {
        bounds.tighten(entry.bounds);
        insert(entry.children, partition, bounds);
        return;
      }
    } else if (generalizes(partition, entry.partition)) {
      Entry new_entry = {
          .partition = partition, .bounds = bounds, .children = {}
      };
      transfer_generalized(entries, new_entry);
      tighten_child_bounds(new_entry);
      entries.emplace_back(std::move(new_entry));
      stats_.insert_misses++;
      stats_.entries++;
      return;
    }
  }

  Entry &entry    = entries.emplace_back();
  entry.partition = partition;
  entry.bounds    = bounds;
  assert(entry.children.size() == 0);
  stats_.insert_misses++;
  stats_.entries++;
}

void TpnBucket::check_invariants() const {
  for (const Entry &entry : entries_) {
    check_invariants(entry);
  }
}

void TpnBucket::check_invariants(const Entry &entry) const {
  assert(entry.bounds.lower_bound <= entry.bounds.upper_bound);
  for (const Entry &child : entry.children) {
    assert(generalizes(entry.partition, child.partition));
    assert(entry.partition != child.partition);
    assert(child.bounds.tighter_or_eq(entry.bounds));
    assert(entry.bounds != child.bounds);
    check_invariants(child);
  }
}

void TpnBucket::tighten_child_bounds(Entry &entry) {
  for (std::size_t i = 0; i < entry.children.size(); i++) {
    stats_.insert_reads++;
    auto &child = entry.children[i];
    if (!child.bounds.tighter(entry.bounds)) {
      child.bounds.tighten(entry.bounds);
      tighten_child_bounds(child);
      if (child.bounds == entry.bounds) {
        for (auto &grandchild : child.children) {
          entry.children.emplace_back(std::move(grandchild));
        }
        remove_at(entry.children, i);
        i--;
        stats_.entries--;
      }
    }
  }
}

void TpnBucket::remove_at(std::vector<Entry> &entries, std::size_t i) {
  assert(i < entries.size());
  if (i == entries.size() - 1) {
    entries[i] = Entry();
    entries.pop_back();
  } else {
    entries[i] = std::move(entries[entries.size() - 1]);
    entries.pop_back();
  }
}

std::ostream &operator<<(std::ostream &os, const TpnBucketKey &key) {
  uint64_t bits = key.bits_;
  os << std::hex;
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
      os << (bits & 0b111);
      bits >>= 3;
    }
  }
  os << (Seat)bits;
  os << std::dec;
  return os;
}

bool TpnTable::lookup(int alpha, int beta, int &score, Cards &winners_by_rank)
    const {
  TpnBucketKey key(game_.next_seat(), game_.normalized_hands());
  auto         it = table_.find(key);
  if (it != table_.end()) {
    alpha -= game_.tricks_taken_by_ns();
    beta -= game_.tricks_taken_by_ns();
    if (it->second.lookup(
            game_.normalized_hands(), alpha, beta, score, winners_by_rank
        )) {
      score += game_.tricks_taken_by_ns();
      winners_by_rank = game_.denormalize_wbr(winners_by_rank);
      return true;
    }
  }
  return false;
}

void TpnTable::insert(Cards winners_by_rank, int lower_bound, int upper_bound) {
  lower_bound -= game_.tricks_taken_by_ns();
  upper_bound -= game_.tricks_taken_by_ns();
  const Hands &hands = game_.normalized_hands();
  winners_by_rank    = game_.normalize_wbr(winners_by_rank);
  Hands partition    = hands.make_partition(winners_by_rank);

  TpnBucketKey key(game_.next_seat(), hands);
  table_[key].insert(partition, lower_bound, upper_bound);
}

TpnTable::Stats TpnTable::stats() const {
  Stats stats;

  stats.lookup_misses += lookup_misses_;
  stats.insert_misses += insert_misses_;

  for (auto &entry : table_) {
    auto &bucket_stats = entry.second.stats();
    stats.buckets++;
    stats.entries += bucket_stats.entries;
    stats.lookup_hits += bucket_stats.lookup_hits;
    stats.lookup_misses += bucket_stats.lookup_misses;
    stats.lookup_reads += bucket_stats.lookup_reads;
    stats.insert_hits += bucket_stats.insert_hits;
    stats.insert_misses += bucket_stats.insert_misses;
    stats.insert_reads += bucket_stats.insert_reads;
  }

  return stats;
}

void TpnTable::check_invariants() const {
  for (auto &entry : table_) {
    entry.second.check_invariants();
  }
}

void TpnBucket::Bounds::tighten(TpnBucket::Bounds bounds) {
  assert(bounds.lower_bound <= upper_bound);
  assert(bounds.upper_bound >= lower_bound);
  lower_bound = std::max(lower_bound, bounds.lower_bound);
  upper_bound = std::min(upper_bound, bounds.upper_bound);
}

bool TpnBucket::Bounds::tighter(TpnBucket::Bounds bounds) const {
  return tighter_or_eq(bounds) && *this != bounds;
}

bool TpnBucket::Bounds::tighter_or_eq(TpnBucket::Bounds bounds) const {
  return lower_bound >= bounds.lower_bound && upper_bound <= bounds.upper_bound;
}

TpnBucketKey::TpnBucketKey(Seat next_seat, const Hands &hands)
    : bits_(next_seat) {
  for (Seat seat = LAST_SEAT; seat >= FIRST_SEAT; seat--) {
    Cards hand = hands.hand(seat);
    for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
      bits_ = (bits_ << 3) | hand.intersect(suit).count();
    }
  }
}

TpnTable::TpnTable(const Game &game)
    : game_(game),
      lookup_misses_(0),
      insert_misses_(0) {}
