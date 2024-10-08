#include "abs_solver.h"
#include "solver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(AbsBounds, empty) {
  AbsBounds b;
  EXPECT_EQ(b.lower(), 13);
  EXPECT_EQ(b.upper(), 0);
}

TEST(AbsBounds, extend) {
  AbsBounds b(1, 3);
  b.extend(AbsBounds(2, 4));
  EXPECT_EQ(b.lower(), 1);
  EXPECT_EQ(b.upper(), 4);
}

TEST(AbsBounds, extend_empty) {
  AbsBounds b;
  b.extend(AbsBounds());
  EXPECT_TRUE(b.empty());
  b.extend(AbsBounds(1, 3));
  EXPECT_FALSE(b.empty());
  EXPECT_EQ(b.lower(), 1);
  EXPECT_EQ(b.upper(), 3);
}

TEST(AbsBounds, max) {
  AbsBounds b1 = AbsBounds(3, 6);
  AbsBounds b2 = AbsBounds(4, 8);
  AbsBounds b  = b1.max(b2);
  EXPECT_EQ(b.lower(), 4);
  EXPECT_EQ(b.upper(), 8);
}

TEST(AbsBounds, max_empty) {
  AbsBounds b1 = AbsBounds(3, 6);
  AbsBounds b2;
  EXPECT_EQ(b1.max(b2), b1);
  EXPECT_EQ(b2.max(b1), b1);
  EXPECT_EQ(b2.max(b2), b2);
}

TEST(AbsBounds, min) {
  AbsBounds b1 = AbsBounds(3, 6);
  AbsBounds b2 = AbsBounds(4, 8);
  AbsBounds b  = b1.min(b2);
  EXPECT_EQ(b.lower(), 3);
  EXPECT_EQ(b.upper(), 6);
}

TEST(AbsBounds, min_empty) {
  AbsBounds b1 = AbsBounds(3, 6);
  AbsBounds b2;
  EXPECT_EQ(b1.min(b2), b1);
  EXPECT_EQ(b2.min(b1), b1);
  EXPECT_EQ(b2.min(b2), b2);
}

struct AbsSolverTestCase {
  const char *name;
  AbsCards    west;
  AbsCards    north;
  AbsCards    east;
  AbsCards    south;
  Suit        trump_suit;
  Seat        lead_seat;
  AbsBounds   bounds;
};

struct AbsSolverTestCase ABS_SOLVER_TEST_CASES[] = {
    {
        .name       = "simple",
        .west       = AbsCards("♠ KX ♥ - ♦ - ♣ -"),
        .north      = AbsCards("♠ X  ♥ A ♦ - ♣ -"),
        .east       = AbsCards("♠ X  ♥ X ♦ - ♣ -"),
        .south      = AbsCards("♠ X  ♥ K ♦ - ♣ -"),
        .trump_suit = NO_TRUMP,
        .lead_seat  = WEST,
        .bounds     = AbsBounds(0, 0),
    },
    {
        .name       = "simple_squeeze",
        .west       = AbsCards("♠ KQ ♥ A   ♦ - ♣ -"),
        .north      = AbsCards("♠ AX ♥ K   ♦ - ♣ -"),
        .east       = AbsCards("♠ -  ♥ XXX ♦ - ♣ -"),
        .south      = AbsCards("♠ X  ♥ X   ♦ - ♣ X"),
        .trump_suit = NO_TRUMP,
        .lead_seat  = SOUTH,
        .bounds     = AbsBounds(3, 3),
    },
};

class AbsSolverTest : public testing::TestWithParam<AbsSolverTestCase> {};

TEST_P(AbsSolverTest, test_case) {
  const auto &tc = GetParam();
  AbsGame     game(
      tc.trump_suit,
      tc.lead_seat,
      (AbsCards[4]){tc.west, tc.north, tc.east, tc.south}
  );
  AbsSolver solver(game);
  AbsBounds bounds = solver.solve().bounds;
  EXPECT_EQ(bounds, tc.bounds);
}

INSTANTIATE_TEST_SUITE_P(
    AbsSolver,
    AbsSolverTest,
    testing::ValuesIn(ABS_SOLVER_TEST_CASES),
    [](const auto &info) { return info.param.name; }
);

void test_random_deal(int seed) {
  std::default_random_engine random(seed);
  Game                       game     = Game::random_deal(random, 4);
  AbsGame                    abs_game = AbsGame::from_game(game, Rank::TEN);
  Solver                     solver(game);
  AbsSolver                  abs_solver(abs_game);
  auto                       res1 = solver.solve();
  auto                       res2 = abs_solver.solve();
  EXPECT_TRUE(res2.bounds.contains(res1.tricks_taken_by_ns));

  // std::cout << res1.tricks_taken_by_ns << ' ' << res2.bounds << ' '
  //           << res1.states_explored << ' ' << res2.states_explored <<
  //           std::endl;
}

TEST(AbsSolver, random_deal) {
  for (int i = 0; i < 100; i++) {
    SCOPED_TRACE(testing::Message() << "seed " << i);
    test_random_deal(i);
  }
}