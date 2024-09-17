#include "game_solver.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(GameSolver, alpha_beta_pruning) {
  std::default_random_engine random(123);

  for (int i = 0; i < 200; i++) {
    Game g = Game::random_deal(random, 3);
    GameSolver gs1 = GameSolver(g, true);
    GameSolver gs2 = GameSolver(g, false);
    int v1 = gs1.solve();
    int v2 = gs2.solve();
    EXPECT_EQ(v1, v2);
  }
}
