#include "abs_solver.h"
#include "solver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(PossTricks, empty) {
  PossTricks p;
  EXPECT_EQ(p.lower_bound(), 0);
  EXPECT_EQ(p.upper_bound(), 13);
  for (int i = 0; i < 13; i++) {
    EXPECT_FALSE(p.contains(i));
  }
}

TEST(PossTricks, only) {
  PossTricks p = PossTricks::only(5);
  EXPECT_EQ(p.lower_bound(), 5);
  EXPECT_EQ(p.upper_bound(), 5);
  for (int i = 0; i < 13; i++) {
    EXPECT_EQ(p.contains(i), i == 5);
  }
}

TEST(PossTricks, union_with) {
  PossTricks p1 = PossTricks::between(1, 3);
  PossTricks p2 = PossTricks::between(6, 8);
  PossTricks p  = p1.union_with(p2);
  EXPECT_EQ(p.lower_bound(), 1);
  EXPECT_EQ(p.upper_bound(), 8);
  for (int i = 0; i < 13; i++) {
    EXPECT_EQ(p.contains(i), (i >= 1 && i <= 3) || (i >= 6 && i <= 8));
  }
}

TEST(PossTricks, max) {
  PossTricks p1 = PossTricks::between(3, 6);
  PossTricks p2 = PossTricks::between(4, 8);
  PossTricks p  = p1.max(p2);
  EXPECT_EQ(p.lower_bound(), 4);
  EXPECT_EQ(p.upper_bound(), 8);
  for (int i = 0; i < 13; i++) {
    EXPECT_EQ(p.contains(i), i >= 4 && i <= 8);
  }
}

TEST(PossTricks, max_empty) {
  PossTricks p1 = PossTricks::between(3, 6);
  PossTricks p2;
  EXPECT_EQ(p1.max(p2), p1);
  EXPECT_EQ(p2.max(p1), p1);
}

TEST(PossTricks, min) {
  PossTricks p1 = PossTricks::between(3, 6);
  PossTricks p2 = PossTricks::between(4, 8);
  PossTricks p  = p1.min(p2);
  EXPECT_EQ(p.lower_bound(), 3);
  EXPECT_EQ(p.upper_bound(), 6);
  for (int i = 0; i < 13; i++) {
    EXPECT_EQ(p.contains(i), i >= 3 && i <= 6);
  }
}

TEST(PossTricks, min_empty) {
  PossTricks p1 = PossTricks::between(3, 6);
  PossTricks p2;
  EXPECT_EQ(p1.min(p2), p1);
  EXPECT_EQ(p2.min(p1), p1);
}

struct AbsSolverTestCase {
  const char *name;
  AbsCards    west;
  AbsCards    north;
  AbsCards    east;
  AbsCards    south;
  Suit        trump_suit;
  Seat        lead_seat;
  PossTricks  poss_tricks;
};

struct AbsSolverTestCase ABS_SOLVER_TEST_CASES[] = {
    {
        .name        = "simple",
        .west        = AbsCards("♠ KX ♥ - ♦ - ♣ -"),
        .north       = AbsCards("♠ X  ♥ A ♦ - ♣ -"),
        .east        = AbsCards("♠ X  ♥ X ♦ - ♣ -"),
        .south       = AbsCards("♠ X  ♥ K ♦ - ♣ -"),
        .trump_suit  = NO_TRUMP,
        .lead_seat   = WEST,
        .poss_tricks = PossTricks::only(0),
    },
    {
        .name        = "simple_squeeze",
        .west        = AbsCards("♠ KQ ♥ A   ♦ - ♣ -"),
        .north       = AbsCards("♠ AX ♥ K   ♦ - ♣ -"),
        .east        = AbsCards("♠ -  ♥ XXX ♦ - ♣ -"),
        .south       = AbsCards("♠ X  ♥ X   ♦ - ♣ X"),
        .trump_suit  = NO_TRUMP,
        .lead_seat   = SOUTH,
        .poss_tricks = PossTricks::only(3),
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
  AbsSolver  solver(game);
  PossTricks poss_tricks = solver.solve().poss_tricks;
  EXPECT_EQ(poss_tricks, tc.poss_tricks);
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
  EXPECT_TRUE(res2.poss_tricks.contains(res1.tricks_taken_by_ns));
}

TEST(AbsSolver, random_deal) {
  for (int i = 0; i < 100; i++) {
    SCOPED_TRACE(testing::Message() << "seed " << i);
    test_random_deal(i);
  }
}