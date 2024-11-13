#include "tpn_table.h"

static bool generalizes(const Hands &partition1, const Hands &partition2) {
  return partition2.contains_all(partition1);
}

bool TpnBucket::lookup(
    const Slice<Entry> &slice,
    const Hands        &hands,
    int                 alpha,
    int                 beta,
    int                &lower_bound,
    int                &upper_bound
) const {
  for (auto &entry : slice) {
    if (hands.contains_all(entry.partition)) {
      assert(entry.bounds.lower_bound <= upper_bound);
      assert(entry.bounds.upper_bound >= lower_bound);
      lower_bound = std::max(lower_bound, (int)entry.bounds.lower_bound);
      upper_bound = std::min(upper_bound, (int)entry.bounds.upper_bound);
      if (lower_bound == upper_bound || lower_bound >= beta ||
          upper_bound <= alpha) {
        return true;
      }
    }
    if (lookup(entry.children, hands, alpha, beta, lower_bound, upper_bound)) {
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
  if (bounds.lower_bound == MIN_BOUND && bounds.upper_bound == MAX_BOUND) {
    return;
  }

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

void TpnTable2::lookup(int alpha, int beta, int &lower_bound, int &upper_bound)
    const {
  TpnBucketKey key(game_.next_seat(), game_.normalized_hands());
  auto         it = table_.find(key);
  if (it == table_.end()) {
    lower_bound = TpnBucket::MIN_BOUND;
    upper_bound = TpnBucket::MAX_BOUND;
  } else {
    alpha -= game_.tricks_taken_by_ns();
    beta -= game_.tricks_taken_by_ns();
    it->second.lookup(
        game_.normalized_hands(), alpha, beta, lower_bound, upper_bound
    );
    lower_bound += game_.tricks_taken_by_ns();
    upper_bound += game_.tricks_taken_by_ns();
  }
}

void TpnTable2::insert(
    Cards winners_by_rank, int lower_bound, int upper_bound
) {
  lower_bound -= game_.tricks_taken_by_ns();
  upper_bound -= game_.tricks_taken_by_ns();
  const Hands &hands = game_.normalized_hands();
  winners_by_rank    = game_.normalize_wbr(winners_by_rank);
  Hands partition    = hands.make_partition(winners_by_rank);

  TpnBucketKey key(game_.next_seat(), hands);
  table_[key].insert(partition, lower_bound, upper_bound);
}

bool TpnTable::lookup_value(int max_depth, Value &value) const {
  bool found = lookup_value_normed(max_depth, value);

  value.lower_bound += game_.tricks_taken_by_ns();
  value.upper_bound += game_.tricks_taken_by_ns();
  if (value.pv_play.has_value()) {
    value.pv_play = game_.denormalize_card(*value.pv_play);
  }

  return found;
}

void TpnTable::upsert_value(int max_depth, const Value &value) {
  Value normed_value = {
      .lower_bound = value.lower_bound - game_.tricks_taken_by_ns(),
      .upper_bound = value.upper_bound - game_.tricks_taken_by_ns(),
  };
  if (value.pv_play.has_value()) {
    normed_value.pv_play = game_.normalize_card(*value.pv_play);
  }

  upsert_value_normed(max_depth, normed_value);
}

bool TpnTable::lookup_value_normed(int max_depth, Value &value) const {
  auto it = table_.find(game_.normalized_state());
  if (it != table_.end() && it->second.max_depth >= max_depth) {
    const Entry &entry = it->second;
    value.lower_bound  = entry.lower_bound;
    value.upper_bound  = entry.upper_bound;
    value.pv_play      = entry.pv_play;
    return true;
  } else {
    value.lower_bound = 0;
    value.upper_bound = game_.tricks_left();
    value.pv_play     = std::nullopt;
    return false;
  }
}

void TpnTable::upsert_value_normed(int max_depth, const Value &value) {
  auto &state = game_.normalized_state();
  auto  it    = table_.find(state);
  if (it != table_.end()) {
    Entry &entry      = it->second;
    entry.lower_bound = (int8_t)value.lower_bound;
    entry.upper_bound = (int8_t)value.upper_bound;
    entry.max_depth   = (int8_t)max_depth;
    entry.pv_play     = value.pv_play;
  } else {
    table_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(state),
        std::forward_as_tuple(
            (int8_t)value.lower_bound,
            (int8_t)value.upper_bound,
            (int8_t)max_depth,
            value.pv_play
        )
    );
  }
}
