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
          states_memoized_(transposition_table_size) {}

    int  tricks_taken_by_ns() const { return tricks_taken_by_ns_; }
    Card best_play() const { return best_play_; }
    int  states_explored() const { return states_explored_; }
    int  states_memoized() const { return states_memoized_; }

  private:
    int  tricks_taken_by_ns_;
    Card best_play_;
    int  states_explored_;
    int  states_memoized_;
  };

  struct GameState {
    std::array<Cards, 4> hands;
    Seat                 next_seat;
    uint8_t              alpha;
    uint8_t              beta;

    void init(const Game &g, int alpha, int beta, bool normalize);

    template <typename H> friend H AbslHashValue(H h, const GameState &s) {
      return H::combine(std::move(h), s.hands, s.next_seat, s.alpha, s.beta);
    }

    friend bool operator==(const GameState &s1, const GameState &s2) {
      return s1.hands == s2.hands && s1.next_seat == s2.next_seat &&
             s1.alpha == s2.alpha && s1.beta == s2.beta;
    }
  };

  Solver(Game g);
  ~Solver();

  void enable_all_optimizations(bool enabled) {
    ab_pruning_enabled_    = enabled;
    tp_table_enabled_      = enabled;
    tp_table_norm_enabled_ = enabled;
    move_ordering_enabled_ = enabled;
    sure_tricks_enabled_   = enabled;
  }

  void enable_ab_pruning(bool enabled) { ab_pruning_enabled_ = enabled; }
  void enable_tp_table(bool enabled) { tp_table_enabled_ = enabled; }
  void enable_tp_table_norm(bool enabled) { tp_table_norm_enabled_ = enabled; }
  void enable_move_ordering(bool enabled) { move_ordering_enabled_ = enabled; }
  void enable_sure_tricks(bool enabled) { sure_tricks_enabled_ = enabled; }

  void enable_tracing(std::ostream *os) {
    trace_ostream_ = os;
    trace_lineno_  = 0;
  }

  Game       &game() { return game_; }
  const Game &game() const { return game_; }

  Result solve();

private:
  int solve_internal(int alpha, int beta, Card *best_play);

  struct SearchState {
    Cards already_searched;
    bool  maximizing;
    int   alpha;
    int   beta;
    int   best_tricks_by_ns;
    Card *best_play;
  };

  enum Order { LOW_TO_HIGH, HIGH_TO_LOW };

  bool search_all_cards(SearchState &s);
  bool search_specific_cards(SearchState &s, Cards c, Order o);
  bool search_specific_card(SearchState &s, Card c);

  int prune_sure_tricks(
      const GameState &s, bool maximizing, int alpha, int beta
  );
  int count_sure_tricks(const GameState &s) const;

  void trace(
      const char      *tag,
      const GameState *state,
      int              alpha,
      int              beta,
      int              tricks_taken_by_ns
  );

  typedef absl::flat_hash_map<GameState, uint8_t> TranspositionTable;

  friend class Searcher;

  Game               game_;
  int                states_explored_;
  TranspositionTable tp_table_;
  bool               ab_pruning_enabled_;
  bool               tp_table_enabled_;
  bool               tp_table_norm_enabled_;
  bool               move_ordering_enabled_;
  bool               sure_tricks_enabled_;
  std::ostream      *trace_ostream_;
  int                trace_lineno_;
};
