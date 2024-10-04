#include "mini_solver.h"
#include "solver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct MiniSolverTestCase {
  const char *name;
  Cards       west;
  Cards       north;
  Cards       east;
  Cards       south;
  Suit        trump_suit;
  Seat        lead_seat;
  int         forced_tricks;
};

class MiniSolverTest : public testing::TestWithParam<MiniSolverTestCase> {};

TEST_P(MiniSolverTest, test_case) {
  const MiniSolverTestCase &p        = GetParam();
  Cards                     hands[4] = {p.west, p.north, p.east, p.south};
  Game                      g(p.trump_suit, p.lead_seat, hands);
  MiniSolver                s(g);
  int                       forced_tricks = s.count_forced_tricks();
  EXPECT_EQ(forced_tricks, p.forced_tricks);
}

const MiniSolverTestCase MINI_SOLVER_TEST_CASES[] = {
    {
        .name          = "no_winners_too_low",
        .west          = Cards("♠ 2 ♥ - ♦ - ♣ -"),
        .north         = Cards("♠ 3 ♥ - ♦ - ♣ -"),
        .east          = Cards("♠ 4 ♥ - ♦ - ♣ -"),
        .south         = Cards("♠ 5 ♥ - ♦ - ♣ -"),
        .trump_suit    = NO_TRUMP,
        .lead_seat     = WEST,
        .forced_tricks = 0,
    },
    {
        .name          = "no_winners_opp_can_ruff",
        .west          = Cards("♠ A ♥ - ♦ - ♣ -"),
        .north         = Cards("♠ - ♥ 2 ♦ - ♣ -"),
        .east          = Cards("♠ 4 ♥ - ♦ - ♣ -"),
        .south         = Cards("♠ 5 ♥ - ♦ - ♣ -"),
        .trump_suit    = HEARTS,
        .lead_seat     = WEST,
        .forced_tricks = 0,
    },
    {
        .name          = "my_winners",
        .west          = Cards("♠ QJ9 ♥ -   ♦ - ♣ -"),
        .north         = Cards("♠ 432 ♥ -   ♦ - ♣ -"),
        .east          = Cards("♠ -   ♥ 432 ♦ - ♣ -"),
        .south         = Cards("♠ 765 ♥ -   ♦ - ♣ -"),
        .trump_suit    = NO_TRUMP,
        .lead_seat     = WEST,
        .forced_tricks = 3,
    },
    {
        .name          = "partner_winners",
        .west          = Cards("♠ 2  ♥ 3 ♦ - ♣ -"),
        .north         = Cards("♠ -  ♥ AK ♦ - ♣ -"),
        .east          = Cards("♠ AK ♥ - ♦ - ♣ -"),
        .south         = Cards("♠ 3  ♥ 4 ♦ - ♣ -"),
        .trump_suit    = NO_TRUMP,
        .lead_seat     = WEST,
        .forced_tricks = 2,
    },
};

INSTANTIATE_TEST_SUITE_P(
    MiniSolver,
    MiniSolverTest,
    testing::ValuesIn(MINI_SOLVER_TEST_CASES),
    [](const auto &info) { return info.param.name; }
);

TEST(MiniSolver, random_test) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game       g  = Game::random_deal(random, 6);
    Solver     s1 = Solver(g);
    MiniSolver s2 = MiniSolver(g);
    auto       r1 = s1.solve();
    int        r2 = s2.count_forced_tricks();

    int best_tricks = g.lead_seat() == NORTH || g.lead_seat() == SOUTH
                          ? r1.tricks_taken_by_ns
                          : r1.tricks_taken_by_ew;
    EXPECT_LE(r2, best_tricks);
  }
}
