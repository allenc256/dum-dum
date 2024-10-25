#pragma once

#include "game_model.h"

#include <absl/container/flat_hash_map.h>

class TpnTable {
public:
  struct Value {
    int                 lower_bound;
    int                 upper_bound;
    std::optional<Card> pv_play;
  };

  TpnTable(const Game &game) : game_(game) {}

  bool   lookup_value(int max_depth, Value &value) const;
  void   upsert_value(int max_depth, const Value &value);
  size_t size() const { return table_.size(); }

private:
  struct Entry {
    int8_t              lower_bound;
    int8_t              upper_bound;
    int8_t              max_depth;
    std::optional<Card> pv_play;

    Entry(
        int8_t              lower_bound,
        int8_t              upper_bound,
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
};
