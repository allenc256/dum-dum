#pragma once

#include "game_model.h"

class GameSolver {
public:
  GameSolver(Game g)
      : game_(g), states_explored_(0), alpha_beta_pruning_enabled_(true) {}

  GameSolver(Game g, bool alpha_beta_pruning_enabled)
      : game_(g), states_explored_(0),
        alpha_beta_pruning_enabled_(alpha_beta_pruning_enabled) {}

  int states_explored() const { return states_explored_; }

  int solve();

private:
  int solve_internal(int depth, int alpha, int beta);

  Game game_;
  int states_explored_;
  bool alpha_beta_pruning_enabled_;
};
