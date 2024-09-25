#include "game_solver.h"
#include <absl/hash/hash_testing.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

constexpr int DEAL_SIZE = 4;

TEST(State, hash) {
  std::default_random_engine      random(123);
  std::uniform_int_distribution<> dist(0, 13);
  std::vector<Solver::State>      states;
  for (int i = 0; i < 100; i++) {
    Game          g = Game::random_deal(random, 13);
    Solver::State s;
    s.init(g, dist(random), dist(random), false);
    states.push_back(s);
  }
  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly(states));
}

void validate_solvers(Solver &s1, Solver &s2) {
  auto r1 = s1.solve();
  auto r2 = s2.solve();
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

TEST(Solver, alpha_beta_pruning) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g  = Game::random_deal(random, DEAL_SIZE);
    Solver s1 = Solver(g);
    Solver s2 = Solver(g);
    s1.enable_all_optimizations(false);
    s1.enable_alpha_beta_pruning(true);
    s2.enable_all_optimizations(false);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solvers(s1, s2);
    });
  }
}

TEST(Solver, transposition_table) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g  = Game::random_deal(random, DEAL_SIZE);
    Solver s1 = Solver(g);
    Solver s2 = Solver(g);
    s1.enable_all_optimizations(false);
    s1.enable_transposition_table(true);
    s2.enable_all_optimizations(false);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solvers(s1, s2);
    });
  }
}

TEST(Solver, state_normalization) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g  = Game::random_deal(random, DEAL_SIZE);
    Solver s1 = Solver(g);
    Solver s2 = Solver(g);
    s1.enable_all_optimizations(false);
    s1.enable_transposition_table(true);
    s1.enable_state_normalization(true);
    s2.enable_all_optimizations(false);
    s2.enable_transposition_table(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solvers(s1, s2);
    });
  }
}

TEST(Solver, all_optimizations) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g  = Game::random_deal(random, DEAL_SIZE);
    Solver s1 = Solver(g);
    Solver s2 = Solver(g);
    s1.enable_all_optimizations(true);
    s2.enable_all_optimizations(false);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solvers(s1, s2);
    });
  }
}
