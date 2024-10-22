#include "tpn_table.h"

bool TpnTable2::lookup_value(int alpha, int beta, int max_depth, Value &value) {
  int  normed_alpha = alpha - game_.tricks_taken_by_ns();
  int  normed_beta  = beta - game_.tricks_taken_by_ns();
  bool found = lookup_value_normed(normed_alpha, normed_beta, max_depth, value);
  denorm_value(value);
  return found;
}

void TpnTable2::upsert_value(int max_depth, const Value &value) {
  Value normed = value;
  norm_value(normed);
  upsert_value_normed(max_depth, normed);
}

bool TpnTable2::lookup_value_normed(
    int alpha, int beta, int max_depth, Value &value
) const {
  value.level       = {};
  value.lower_bound = 0;
  value.upper_bound = to_int8_t(game_.tricks_left());
  value.pv_play     = std::nullopt;

  SeatShapes key(game_);
  auto       range = multimap_.equal_range(key);
  for (auto i = range.first; i != range.second; ++i) {
    auto &entry = i->second;
    if (entry.state.matches(game_) && entry.max_depth >= max_depth) {
      if (entry.lower_bound == entry.upper_bound) {
        value.level       = entry.state.level();
        value.lower_bound = entry.lower_bound;
        value.upper_bound = entry.upper_bound;
        value.pv_play     = entry.pv_play;
        return true;
      }

      if (entry.lower_bound > value.lower_bound) {
        value.lower_bound = entry.lower_bound;
        value.level.intersect(entry.state.level());
        if (value.lower_bound >= beta) {
          return true;
        }
      }

      if (entry.upper_bound < value.upper_bound) {
        value.upper_bound = entry.upper_bound;
        value.level.intersect(entry.state.level());
        if (value.upper_bound <= alpha) {
          return true;
        }
      }
    }
  }

  return false;
}

void TpnTable2::upsert_value_normed(int max_depth, const Value &value) {
  AbsState   state = {value.level, game_};
  SeatShapes key   = {game_};
  auto       range = multimap_.equal_range(key);
  for (auto i = range.first; i != range.second; ++i) {
    Entry &entry = i->second;
    if (state == entry.state) {
      entry.lower_bound = value.lower_bound;
      entry.upper_bound = value.upper_bound;
      entry.pv_play     = value.pv_play;
      entry.max_depth   = to_int8_t(max_depth);
      return;
    }
  }
  multimap_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(key),
      std::forward_as_tuple(
          state, value.lower_bound, value.upper_bound, max_depth, value.pv_play
      )
  );
}

void TpnTable2::norm_value(Value &value) {
  // value.level.normalize(game_);
  value.lower_bound -= game_.tricks_taken_by_ns();
  value.upper_bound -= game_.tricks_taken_by_ns();
  // if (value.pv_play.has_value()) {
  //   value.pv_play = game_.normalize_card(*value.pv_play);
  // }
}

void TpnTable2::denorm_value(Value &value) {
  // value.level.denormalize(game_);
  value.lower_bound += game_.tricks_taken_by_ns();
  value.upper_bound += game_.tricks_taken_by_ns();
  // if (value.pv_play.has_value()) {
  //   value.pv_play = game_.denormalize_card(*value.pv_play);
  // }
}
