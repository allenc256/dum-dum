#pragma once

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

class TpnTable {
public:
  struct Value {
    float               lower_bound;
    float               upper_bound;
    std::optional<Card> pv_play;
  };

  struct Stats {
    int states_by_depth[13] = {0};
  };

  TpnTable(const Game &game) : game_(game) {}

  bool         lookup_value(int max_depth, Value &value) const;
  void         upsert_value(int max_depth, const Value &value);
  size_t       size() const { return table_.size(); }
  const Stats &stats() const { return stats_; }

private:
  struct Entry {
    float               lower_bound;
    float               upper_bound;
    int8_t              max_depth;
    std::optional<Card> pv_play;

    Entry(
        float               lower_bound,
        float               upper_bound,
        int8_t              max_depth,
        std::optional<Card> pv_play
    )
        : lower_bound(lower_bound),
          upper_bound(upper_bound),
          max_depth(max_depth),
          pv_play(pv_play) {}
  };

  using HashTable = absl::flat_hash_map<Game::State, Entry>;

  bool lookup_value_normed(int max_depth, Value &value) const;
  void upsert_value_normed(int max_depth, const Value &value);

  const Game &game_;
  HashTable   table_;
  Stats       stats_;
};
