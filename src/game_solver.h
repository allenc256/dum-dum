#pragma once

#include <absl/container/flat_hash_map.h>

#include <array>
#include <vector>

#include "game_model.h"

class Solver {
public:
  class Result {
  public:
    Result(
        int  tricks_taken_by_ns,
        Card best_play,
        int  states_explored,
        int  transposition_table_size
    )
        : tricks_taken_by_ns_(tricks_taken_by_ns),
          best_play_(best_play),
          states_explored_(states_explored),
          transposition_table_size_(transposition_table_size) {}

    int  tricks_taken_by_ns() const { return tricks_taken_by_ns_; }
    Card best_play() const { return best_play_; }
    int  states_explored() const { return states_explored_; }
    int  transposition_table_size() const { return transposition_table_size_; }

  private:
    int  tricks_taken_by_ns_;
    Card best_play_;
    int  states_explored_;
    int  transposition_table_size_;
  };

  struct State {
    std::array<Cards, 4> hands;
    Seat                 next_seat;
    int8_t               alpha;
    int8_t               beta;

    void init(const Game &g, int alpha, int beta, bool normalize);

    template <typename H> friend H AbslHashValue(H h, const State &s) {
      return H::combine(std::move(h), s.hands, s.next_seat, s.alpha, s.beta);
    }

    friend bool operator==(const State &s1, const State &s2) {
      return s1.hands == s2.hands && s1.next_seat == s2.next_seat &&
             s1.alpha == s2.alpha && s1.beta == s2.beta;
    }
  };

  Solver(Game g);
  ~Solver();

  void enable_all_optimizations(bool enabled) {
    alpha_beta_pruning_enabled_  = enabled;
    transposition_table_enabled_ = enabled;
    state_normalization_enabled_ = enabled;
  }

  void enable_alpha_beta_pruning(bool enabled) {
    alpha_beta_pruning_enabled_ = enabled;
  }

  void enable_transposition_table(bool enabled) {
    transposition_table_enabled_ = enabled;
  }

  void enable_state_normalization(bool enabled) {
    state_normalization_enabled_ = enabled;
  }

  Game       &game() { return game_; }
  const Game &game() const { return game_; }

  Result solve();

private:
  int solve_internal(int alpha, int beta, Card *best_play);

  int solve_internal_search_plays(
      bool maximizing, int &alpha, int &beta, Card *best_play
  );

  bool solve_internal_search_single_play(
      Card  play,
      bool  maximizing,
      int  &alpha,
      int  &beta,
      int  &best_tricks_by_ns,
      Card *best_play
  );

  int count_sure_tricks(const State &s) const;

  typedef absl::flat_hash_map<State, uint8_t> TranspositionTable;

  Game               game_;
  int                states_explored_;
  TranspositionTable transposition_table_;
  bool               alpha_beta_pruning_enabled_;
  bool               transposition_table_enabled_;
  bool               state_normalization_enabled_;
};
