#pragma once

#include <absl/container/flat_hash_map.h>

#include <array>
#include <vector>

#include "game_model.h"

class Solver {
public:
  class Result {
  public:
    Result(int tricks_taken_by_ns, Card best_play, int states_explored)
        : tricks_taken_by_ns_(tricks_taken_by_ns),
          best_play_(best_play),
          states_explored_(states_explored) {}

    int  tricks_taken_by_ns() const { return tricks_taken_by_ns_; }
    Card best_play() const { return best_play_; }
    int  states_explored() const { return states_explored_; }

  private:
    int  tricks_taken_by_ns_;
    Card best_play_;
    int  states_explored_;
  };

  struct TTKey {
    std::array<uint64_t, 4> hands_;
    std::array<uint8_t, 4>  trick_cards_;
    uint8_t                 trick_card_count_;
    uint8_t                 trick_lead_seat_;
    int8_t                  alpha_;
    int8_t                  beta_;

    template <typename H> friend H AbslHashValue(H h, const TTKey &s) {
      return H::combine(
          std::move(h),
          s.hands_,
          s.trick_cards_,
          s.trick_card_count_,
          s.trick_lead_seat_,
          s.alpha_,
          s.beta_
      );
    }

    friend bool operator==(const TTKey &s1, const TTKey &s2) {
      return s1.hands_ == s2.hands_ && s1.trick_cards_ == s2.trick_cards_ &&
             s1.trick_card_count_ == s2.trick_card_count_ &&
             s1.trick_lead_seat_ == s2.trick_lead_seat_ &&
             s1.alpha_ == s2.alpha_ && s1.beta_ == s2.beta_;
    }
  };

  Solver(Game g);
  ~Solver();

  void enable_all_optimizations(bool enabled) {
    alpha_beta_pruning_enabled_  = enabled;
    transposition_table_enabled_ = enabled;
    state_normalization_         = enabled;
  }

  void enable_alpha_beta_pruning(bool enabled) {
    alpha_beta_pruning_enabled_ = enabled;
  }

  void enable_transposition_table(bool enabled) {
    transposition_table_enabled_ = enabled;
  }

  void enable_state_normalization(bool enabled) {
    state_normalization_ = enabled;
  }

  void enable_tracing(std::ostream &os);
  void enable_tracing(std::ostream &os, int trace_depth);

  Game       &game() { return game_; }
  const Game &game() const { return game_; }

  Result solve();

  static void
  init_ttkey(TTKey &k, const Game &g, int alpha, int beta, bool normalize);

private:
  int  solve_internal(int alpha, int beta, Card *best_play);
  bool solve_internal_search_plays(
      bool  maximizing,
      int  &alpha,
      int  &beta,
      int  &best_tricks_by_ns,
      Card *best_play
  );
  bool solve_internal_search_single_play(
      Card  play,
      bool  maximizing,
      int  &alpha,
      int  &beta,
      int  &best_tricks_by_ns,
      Card *best_play
  );

  class Tracer;

  typedef absl::flat_hash_map<TTKey, uint8_t> TranspositionTable;

  Game                    game_;
  int                     states_explored_;
  TranspositionTable      transposition_table_;
  bool                    alpha_beta_pruning_enabled_;
  bool                    transposition_table_enabled_;
  bool                    state_normalization_;
  std::unique_ptr<Tracer> tracer_;
};
