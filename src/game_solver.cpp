#include "game_solver.h"

int GameSolver::solve() { return solve_internal(0, 0, game_.tricks_max()); }

int GameSolver::solve_internal(int depth, int alpha, int beta) {
  states_explored_++;

  if (game_.finished()) {
    return game_.tricks_taken_by_ns();
  }

  bool maximizing;
  int best_tricks_by_ns;
  if (game_.next_seat() == NORTH || game_.next_seat() == SOUTH) {
    maximizing = true;
    best_tricks_by_ns = 0;
  } else {
    maximizing = false;
    best_tricks_by_ns = game_.tricks_max();
  }

  Cards valid_plays = game_.valid_plays();
  for (auto i = valid_plays.first(); i.valid(); i = valid_plays.next(i)) {
    Card c = i.card();
    game_.play(c);
    int tricks_by_ns = solve_internal(depth + 1, alpha, beta);
    if (maximizing) {
      best_tricks_by_ns = std::max(best_tricks_by_ns, tricks_by_ns);
      alpha = std::max(alpha, best_tricks_by_ns);
      if (alpha_beta_pruning_enabled_ && best_tricks_by_ns >= beta) {
        game_.unplay();
        break;
      }
    } else {
      best_tricks_by_ns = std::min(best_tricks_by_ns, tricks_by_ns);
      beta = std::min(beta, best_tricks_by_ns);
      if (alpha_beta_pruning_enabled_ && best_tricks_by_ns <= alpha) {
        game_.unplay();
        break;
      }
    }
    game_.unplay();
  }

  return best_tricks_by_ns;
}
