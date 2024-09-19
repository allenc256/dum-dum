#pragma once

#include "game_model.h"
#include <absl/container/flat_hash_map.h>
#include <array>
#include <vector>

struct State {
  std::array<uint64_t, 4> hands;
  std::array<uint8_t, 4> trick;
  uint8_t trick_card_count;
  uint8_t trick_lead_seat;
  uint8_t alpha;
  uint8_t beta;

  State(const Game &g, int alpha, int beta);

  void sha256_hash(uint8_t digest[32]) const;

  template <typename H> friend H AbslHashValue(H h, const State &s) {
    return H::combine(std::move(h), s.hands, s.trick, s.trick_card_count,
                      s.trick_lead_seat, s.alpha, s.beta);
  }

  friend bool operator==(const State &s1, const State &s2) {
    return s1.hands == s2.hands && s1.trick == s2.trick &&
           s1.trick_card_count == s2.trick_card_count &&
           s1.trick_lead_seat == s2.trick_lead_seat && s1.alpha == s2.alpha &&
           s1.beta == s2.beta;
  }
};

class Solver {
public:
  class Result {
  public:
    Result(int tricks_taken_by_ns, Card best_play, int states_explored)
        : tricks_taken_by_ns_(tricks_taken_by_ns), best_play_(best_play),
          states_explored_(states_explored) {}

    int tricks_taken_by_ns() const { return tricks_taken_by_ns_; }
    Card best_play() const { return best_play_; }
    int states_explored() const { return states_explored_; }

  private:
    int tricks_taken_by_ns_;
    Card best_play_;
    int states_explored_;
  };

  Solver(Game g);
  ~Solver();

  void enable_all_optimizations() {
    alpha_beta_pruning_enabled_ = true;
    transposition_table_enabled_ = true;
  }

  void disable_all_optimizations() {
    alpha_beta_pruning_enabled_ = false;
    transposition_table_enabled_ = false;
  }

  void enable_alpha_beta_pruning(bool enabled) {
    alpha_beta_pruning_enabled_ = enabled;
  }

  void enable_transposition_table(bool enabled) {
    transposition_table_enabled_ = enabled;
  }

  void enable_tracing(std::ostream &os);
  void enable_tracing(std::ostream &os, int trace_depth);

  int states_explored() const { return states_explored_; }

  size_t transposition_table_size() const {
    return transposition_table_.size();
  }

  Game &game() { return game_; }
  const Game &game() const { return game_; }

  Result solve();

private:
  int solve_internal(int alpha, int beta, Card *best_play);

  class Tracer;

  Game game_;
  int states_explored_;
  absl::flat_hash_map<State, uint8_t> transposition_table_;
  bool alpha_beta_pruning_enabled_;
  bool transposition_table_enabled_;
  std::unique_ptr<Tracer> tracer_;
};
