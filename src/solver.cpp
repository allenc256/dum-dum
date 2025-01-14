#include "solver.h"
#include "fast_tricks.h"
#include "play_order.h"

Solver::Solver(Game g)
    : game_(g),
      nodes_explored_(0),
      tpn_table_(game_),
      trace_os_(nullptr),
      trace_lineno_(0) {
  enable_all_optimizations(true);
}

Solver::~Solver() {}

Solver::Stats Solver::stats() const {
  return {
      .nodes_explored  = nodes_explored_,
      .tpn_table_stats = tpn_table_.stats(),
  };
}

void Solver::enable_all_optimizations(bool enabled) {
  ab_pruning_enabled_  = enabled;
  tpn_table_enabled_   = enabled;
  play_order_enabled_  = enabled;
  fast_tricks_enabled_ = enabled;
}

void Solver::enable_ab_pruning(bool enabled) { ab_pruning_enabled_ = enabled; }
void Solver::enable_tpn_table(bool enabled) { tpn_table_enabled_ = enabled; }
void Solver::enable_play_order(bool enabled) { play_order_enabled_ = enabled; }

void Solver::enable_fast_tricks(bool enabled) {
  fast_tricks_enabled_ = enabled;
}

void Solver::enable_tracing(std::ostream *os) {
  trace_os_     = os;
  trace_lineno_ = 0;
}

Solver::Result Solver::solve() { return solve(0, game_.tricks_max()); }

Solver::Result Solver::solve(int alpha, int beta) {
  Cards winners_by_rank;
  int   tricks_taken_by_ns = solve_internal(alpha, beta, winners_by_rank);
  int   tricks_taken_by_ew = game_.tricks_max() - tricks_taken_by_ns;
#ifndef NDEBUG
  if (tpn_table_enabled_) {
    tpn_table_.check_invariants();
  }
#endif
  return {
      .tricks_taken_by_ns = tricks_taken_by_ns,
      .tricks_taken_by_ew = tricks_taken_by_ew,
      .winners_by_rank    = winners_by_rank,
  };
}

#define TRACE(tag, alpha, beta, score)                                         \
  if (trace_os_) {                                                             \
    trace(tag, alpha, beta, score);                                            \
  }

int Solver::solve_internal(int alpha, int beta, Cards &winners_by_rank) {
  if (game_.finished()) {
    TRACE("terminal", alpha, beta, game_.tricks_taken_by_ns());
    return game_.tricks_taken_by_ns();
  }

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;

  if (game_.start_of_trick()) {
    if (tpn_table_enabled_) {
      int score;
      if (tpn_table_.lookup(alpha, beta, score, winners_by_rank)) {
        TRACE("tpn_cutoff", alpha, beta, score);
        return score;
      }
    }

    if (fast_tricks_enabled_) {
      int score;
      if (prune_fast_tricks(alpha, beta, score, winners_by_rank)) {
        TRACE("ft_cutoff", alpha, beta, score);
        return score;
      }
    }

    TRACE("start", alpha, beta, -1);
  }

  int best_score = maximizing ? -1 : game_.tricks_max() + 1;
  search_all_cards(alpha, beta, best_score, winners_by_rank);

  if (game_.start_of_trick()) {
    TRACE("end", alpha, beta, best_score);

    if (tpn_table_enabled_) {
      int lower_bound = game_.tricks_taken_by_ns();
      int upper_bound = game_.tricks_taken_by_ns() + game_.tricks_left();
      if (best_score < beta) {
        upper_bound = best_score;
      }
      if (best_score > alpha) {
        lower_bound = best_score;
      }
      tpn_table_.insert(winners_by_rank, lower_bound, upper_bound);
    }
  }

  return best_score;
}

static void add_last_trick_wbr(const Game &game, Cards &winners_by_rank) {
  if (game.start_of_trick()) {
    Cards last_trick_wbr = game.last_trick().winners_by_rank(game.hands());
    winners_by_rank.add_all(last_trick_wbr);
  }
}

void Solver::search_all_cards(
    int alpha, int beta, int &best_score, Cards &winners_by_rank
) {
  nodes_explored_++;

  bool maximizing = game_.next_seat() == NORTH || game_.next_seat() == SOUTH;

  PlayOrder order;
  order_plays(game_, order);

  for (Card c : order) {
    game_.play(c);

    Cards child_winners_by_rank;
    int   child_score = solve_internal(alpha, beta, child_winners_by_rank);

    if (maximizing) {
      if (child_score > best_score) {
        best_score = child_score;
      }
      if (ab_pruning_enabled_) {
        alpha = std::max(alpha, best_score);
        if (best_score >= beta) {
          winners_by_rank = child_winners_by_rank;
          add_last_trick_wbr(game_, winners_by_rank);
          game_.unplay();
          return;
        }
      }
    } else {
      if (child_score < best_score) {
        best_score = child_score;
      }
      if (ab_pruning_enabled_) {
        beta = std::min(beta, best_score);
        if (best_score <= alpha) {
          winners_by_rank = child_winners_by_rank;
          add_last_trick_wbr(game_, winners_by_rank);
          game_.unplay();
          return;
        }
      }
    }

    winners_by_rank.add_all(child_winners_by_rank);
    add_last_trick_wbr(game_, winners_by_rank);
    game_.unplay();
  }
}

bool Solver::prune_fast_tricks(
    int alpha, int beta, int &score, Cards &winners_by_rank
) const {
  int fast_tricks;

  estimate_fast_tricks(
      game_.hands(),
      game_.next_seat(),
      game_.trump_suit(),
      fast_tricks,
      winners_by_rank
  );

  if (game_.next_seat() == NORTH || game_.next_seat() == SOUTH) {
    int lb = game_.tricks_taken_by_ns() + fast_tricks;
    if (lb >= beta) {
      score = lb;
      return true;
    }
  } else {
    int ub = game_.tricks_taken_by_ns() + game_.tricks_left() - fast_tricks;
    if (ub <= alpha) {
      score = ub;
      return true;
    }
  }

  return false;
}

void Solver::trace(const char *tag, int alpha, int beta, int score) {
  std::ostream_iterator<char> out(*trace_os_);

  std::format_to(out, "{:<7} {:<10} {}", trace_lineno_, tag, game_.hands());
  int max_len = game_.tricks_max() * 4 + 15;
  int cur_len = game_.hands().all_cards().count() + 15;
  for (int i = 0; i < max_len - cur_len + 1; i++) {
    std::format_to(out, "{}", ' ');
  }
  std::format_to(
      out, "{:2} {:2} {:2}", alpha, beta, game_.tricks_taken_by_ns()
  );
  if (score >= 0) {
    std::format_to(out, "{:2} ", score);
  } else {
    std::format_to(out, "   ");
  }
  for (int i = 0; i < game_.tricks_taken(); i++) {
    std::format_to(out, "{}", game_.trick(i));
  }
  std::format_to(out, "\n");
  trace_lineno_++;
}
