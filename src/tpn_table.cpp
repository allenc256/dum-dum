#include "tpn_table.h"

static bool generalizes(const Hands &partition1, const Hands &partition2) {
  return partition2.contains_all(partition1);
}

bool TpnBucket::lookup(
    const Slice<Entry> &slice,
    const Hands        &hands,
    int                 alpha,
    int                 beta,
    int                &score,
    Cards              &winners_by_rank
) const {
  for (auto &entry : slice) {
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

void TpnBucket::transfer_generalized(Slice<Entry> &src, Entry &dest) const {
  for (int i = 0; i < src.size(); i++) {
    auto &e = src[i];
    if (generalizes(dest.partition, e.partition)) {
      dest.children.expand() = std::move(e);
      src.remove_at(i);
      i--;
    }
  }
}

void TpnBucket::insert(
    Slice<Entry> &slice, const Hands &partition, Bounds bounds
) {
  for (auto &entry : slice) {
    if (partition == entry.partition) {
      if (!entry.bounds.tighter_or_eq(bounds)) {
        entry.bounds.tighten(bounds);
        tighten_child_bounds(entry);
      }
      return;
    } else if (generalizes(entry.partition, partition)) {
      if (!entry.bounds.tighter_or_eq(bounds)) {
        bounds.tighten(entry.bounds);
        insert(entry.children, partition, bounds);
      }
      return;
    } else if (generalizes(partition, entry.partition)) {
      Entry new_entry = {.partition = partition, .bounds = bounds};
      transfer_generalized(slice, new_entry);
      tighten_child_bounds(new_entry);
      slice.expand() = std::move(new_entry);
      return;
    }
  }

  Entry &entry    = slice.expand();
  entry.partition = partition;
  entry.bounds    = bounds;
  assert(entry.children.size() == 0);
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
  for (int i = 0; i < entry.children.size(); i++) {
    auto &child = entry.children[i];
    if (!child.bounds.tighter(entry.bounds)) {
      child.bounds.tighten(entry.bounds);
      tighten_child_bounds(child);
      if (child.bounds == entry.bounds) {
        for (auto &grandchild : child.children) {
          entry.children.expand() = std::move(grandchild);
        }
        entry.children.remove_at(i);
        i--;
      }
    }
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
