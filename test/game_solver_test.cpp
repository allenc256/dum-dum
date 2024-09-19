#include "game_solver.h"
#include <absl/hash/hash_testing.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(Solver, alpha_beta_pruning) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game g = Game::random_deal(random, 4);
    Solver s1 = Solver(g);
    Solver s2 = Solver(g);
    std::vector<Card> line1;
    std::vector<Card> line2;
    s1.disable_all_optimizations();
    s1.enable_alpha_beta_pruning(true);
    s2.disable_all_optimizations();
    Solver::Result r1 = s1.solve();
    Solver::Result r2 = s2.solve();
    ASSERT_EQ(r1.tricks_taken_by_ns(), r2.tricks_taken_by_ns());
    while (!s1.game().finished()) {
      s1.game().play(r1.best_play());
      r1 = s1.solve();
      ASSERT_EQ(r1.tricks_taken_by_ns(), r2.tricks_taken_by_ns());
    }
    ASSERT_EQ(s1.game().tricks_taken_by_ns(), r1.tricks_taken_by_ns());
  }
}

TEST(Solver, transposition_table) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game g = Game::random_deal(random, 3);
    Solver s1 = Solver(g);
    Solver s2 = Solver(g);
    std::vector<Card> line1;
    std::vector<Card> line2;
    s1.disable_all_optimizations();
    s1.enable_transposition_table(true);
    s2.disable_all_optimizations();
    Solver::Result r1 = s1.solve();
    Solver::Result r2 = s2.solve();
    SCOPED_TRACE(::testing::Message() << "iteration " << i);
    ASSERT_EQ(r1.tricks_taken_by_ns(), r2.tricks_taken_by_ns());
    int count = 0;
    while (!s1.game().finished()) {
      SCOPED_TRACE(::testing::Message() << "card " << count);
      s1.game().play(r1.best_play());
      r1 = s1.solve();
      count++;
      ASSERT_EQ(r1.tricks_taken_by_ns(), r2.tricks_taken_by_ns());
    }
    ASSERT_EQ(s1.game().tricks_taken_by_ns(), r1.tricks_taken_by_ns());
  }
}

TEST(State, hash) {
  std::default_random_engine random(123);
  std::uniform_int_distribution<> d1(0, 3);
  std::uniform_int_distribution<> d2(0, 13);
  std::vector<State> states;
  for (int i = 0; i < 100; i++) {
    Game g = Game::random_deal(random, 13);
    int tricks = d1(random);
    for (int j = 0; j < tricks; j++) {
      g.play(g.valid_plays().first().card());
    }
    int alpha = d2(random);
    int beta = d2(random);
    states.push_back(State(g, alpha, beta));
  }
  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly(states));
}
