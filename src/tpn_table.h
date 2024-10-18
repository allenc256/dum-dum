#pragma once

#include "game_model.h"

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
      : lower_bound_((int8_t)lower_bound),
        upper_bound_((int8_t)upper_bound),
        max_depth_((int8_t)max_depth),
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
      lower_bound_ = (int8_t)lb;
      return true;
    } else {
      return false;
    }
  }

  bool update_upper_bound(int ub) {
    assert(ub >= lower_bound_);
    if (ub < upper_bound_) {
      upper_bound_ = (int8_t)ub;
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
