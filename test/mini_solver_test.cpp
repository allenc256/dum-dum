#include "mini_solver.h"
#include "random.h"
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
  TpnTable                  tpn_table(g);
  MiniSolver                s(g, tpn_table);
  TpnTableValue             v = s.compute_value(g.tricks_max());
  bool maximizing             = p.lead_seat == NORTH || p.lead_seat == SOUTH;
  int  forced_tricks =
      maximizing ? v.lower_bound() : g.tricks_max() - v.upper_bound();
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
        .name          = "partner_winners_west",
        .west          = Cards("♠ 2  ♥ 3 ♦ - ♣ -"),
        .north         = Cards("♠ -  ♥ AK ♦ - ♣ -"),
        .east          = Cards("♠ AK ♥ - ♦ - ♣ -"),
        .south         = Cards("♠ 3  ♥ 4 ♦ - ♣ -"),
        .trump_suit    = NO_TRUMP,
        .lead_seat     = WEST,
        .forced_tricks = 2,
    },
    {
        .name          = "partner_winners_north",
        .west          = Cards("♠ 3  ♥ 4 ♦ - ♣ -"),
        .north         = Cards("♠ 2  ♥ 3 ♦ - ♣ -"),
        .east          = Cards("♠ -  ♥ AK ♦ - ♣ -"),
        .south         = Cards("♠ AK ♥ - ♦ - ♣ -"),
        .trump_suit    = NO_TRUMP,
        .lead_seat     = NORTH,
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
  for (int i = 0; i < 100; i++) {
    Game          g  = Random(i).random_game(6);
    Solver        s1 = Solver(g);
    TpnTable      tpn_table(g);
    MiniSolver    s2 = MiniSolver(g, tpn_table);
    auto          r  = s1.solve();
    TpnTableValue v  = s2.compute_value(g.tricks_max());

    EXPECT_LE(v.lower_bound(), r.tricks_taken_by_ns);
    EXPECT_GE(v.upper_bound(), r.tricks_taken_by_ns);
  }
}
