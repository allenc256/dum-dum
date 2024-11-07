#include "tpn_table.h"

bool TpnTable::lookup_value(int max_depth, Value &value) const {
  bool found = lookup_value_normed(max_depth, value);

  value.lower_bound += (float)game_.tricks_taken_by_ns();
  value.upper_bound += (float)game_.tricks_taken_by_ns();
  if (value.pv_play.has_value()) {
    value.pv_play = game_.denormalize_card(*value.pv_play);
  }

  return found;
}

void TpnTable::upsert_value(int max_depth, const Value &value) {
  Value normed_value = {
      .lower_bound = value.lower_bound - (float)game_.tricks_taken_by_ns(),
      .upper_bound = value.upper_bound - (float)game_.tricks_taken_by_ns(),
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
    value.upper_bound = (float)game_.tricks_left();
    value.pv_play     = std::nullopt;
    return false;
  }
}

void TpnTable::upsert_value_normed(int max_depth, const Value &value) {
  auto &state = game_.normalized_state();
  auto  it    = table_.find(state);
  if (it != table_.end()) {
    Entry &entry      = it->second;
    entry.lower_bound = value.lower_bound;
    entry.upper_bound = value.upper_bound;
    entry.max_depth   = (int8_t)max_depth;
    entry.pv_play     = value.pv_play;
  } else {
    table_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(state),
        std::forward_as_tuple(
            value.lower_bound,
            value.upper_bound,
            (int8_t)max_depth,
            value.pv_play
        )
    );
    stats_.states_by_depth[game_.tricks_taken()]++;
  }
}
