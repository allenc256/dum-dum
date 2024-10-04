#include "solver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

constexpr int DEAL_SIZE = 4;

void validate_solver(Solver &s1) {
  Solver s2 = Solver(s1.game());
  s2.enable_all_optimizations(false);
  auto r1 = s1.solve();
  auto r2 = s2.solve();
  ASSERT_EQ(r1.tricks_taken_by_ns, r2.tricks_taken_by_ns);
  int count = 0;
  while (!s1.game().finished()) {
    SCOPED_TRACE(::testing::Message() << "card " << count);
    s1.game().play(r1.best_play);
    r1 = s1.solve();
    count++;
    ASSERT_EQ(r1.tricks_taken_by_ns, r2.tricks_taken_by_ns);
  }
  ASSERT_EQ(s1.game().tricks_taken_by_ns(), r1.tricks_taken_by_ns);
}

TEST(Solver, ab_pruning) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g = Game::random_deal(random, DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_ab_pruning(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solver(s);
    });
  }
}

TEST(Solver, mini_solver) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g = Game::random_deal(random, DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_ab_pruning(true);
    s.enable_mini_solver(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(
          ::testing::Message() << "iteration " << i << ":" << std::endl
                               << g
      );
      validate_solver(s);
    });
  }
}

TEST(Solver, tp_table) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g = Game::random_deal(random, DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_tp_table(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solver(s);
    });
  }
}

TEST(Solver, tp_table_norm) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g = Game::random_deal(random, DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_tp_table(true);
    s.enable_tp_table_norm(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solver(s);
    });
  }
}

TEST(Solver, move_ordering) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g = Game::random_deal(random, DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_move_ordering(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solver(s);
    });
  }
}

TEST(Solver, all_optimizations) {
  std::default_random_engine random(123);

  for (int i = 0; i < 100; i++) {
    Game   g = Game::random_deal(random, DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "iteration " << i);
      validate_solver(s);
    });
  }
}

struct ManualTestCase {
  const char *name;
  Cards       west;
  Cards       north;
  Cards       east;
  Cards       south;
  Suit        trump_suit;
  Seat        lead_seat;
  int         tricks_taken_by_ns;
  Cards       best_plays;
};

class ManualTest : public testing::TestWithParam<ManualTestCase> {};

TEST_P(ManualTest, manual_test) {
  const ManualTestCase &p        = GetParam();
  Cards                 hands[4] = {p.west, p.north, p.east, p.south};
  Game                  g(p.trump_suit, p.lead_seat, hands);
  Solver                s(g);
  auto                  r = s.solve();
  EXPECT_EQ(r.tricks_taken_by_ns, p.tricks_taken_by_ns);
  EXPECT_TRUE(p.best_plays.contains(r.best_play));
}

const ManualTestCase MANUAL_TESTS[] = {
    {
        // https://en.wikipedia.org/wiki/Simple_squeeze
        .name               = "simple_squeeze",
        .west               = Cards("♠ KQ ♥ A   ♦ - ♣ -"),
        .north              = Cards("♠ AJ ♥ K   ♦ - ♣ -"),
        .east               = Cards("♠ -  ♥ QJT ♦ - ♣ -"),
        .south              = Cards("♠ 4  ♥ 2   ♦ - ♣ A"),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 3,
        .best_plays         = Cards({"A♣"}),
    },
    {
        // https://en.wikipedia.org/wiki/Simple_squeeze
        .name               = "split_two_card_threat_squeeze",
        .west               = Cards("♠ KQ ♥ A   ♦ - ♣ -"),
        .north              = Cards("♠ A3 ♥ K   ♦ - ♣ -"),
        .east               = Cards("♠ -  ♥ QJT ♦ - ♣ -"),
        .south              = Cards("♠ J2 ♥ -   ♦ - ♣ A"),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 3,
        .best_plays         = Cards({"A♣"}),
    },
    {
        // https://en.wikipedia.org/wiki/Simple_squeeze
        .name               = "criss_cross_squeeze",
        .west               = Cards("♠ -  ♥ -  ♦ - ♣ 6543"),
        .north              = Cards("♠ A  ♥ Q2 ♦ - ♣ 2"),
        .east               = Cards("♠ K3 ♥ K3 ♦ - ♣ -"),
        .south              = Cards("♠ Q2 ♥ A  ♦ - ♣ A"),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 4,
        .best_plays         = Cards({"A♣"}),
    },
    {
        // https://en.wikipedia.org/wiki/Simple_squeeze
        .name               = "vienna_coup",
        .west               = Cards("♠ -  ♥ -  ♦ - ♣ 5432"),
        .north              = Cards("♠ AJ ♥ A  ♦ 2 ♣ -"),
        .east               = Cards("♠ KQ ♥ K3 ♦ - ♣ -"),
        .south              = Cards("♠ 2  ♥ Q2 ♦ A ♣ -"),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 4,
        .best_plays         = Cards({"2♥"}),
    },
    {
        // https://en.wikipedia.org/wiki/Trump_squeeze
        .name               = "trump_squeeze",
        .west               = Cards("♠ -   ♥ - ♦ 65432 ♣ -"),
        .north              = Cards("♠ A   ♥ - ♦ A     ♣ KT7"),
        .east               = Cards("♠ Q9  ♥ - ♦ -     ♣ J98"),
        .south              = Cards("♠ T83 ♥ 2 ♦ -     ♣ 3"),
        .trump_suit         = HEARTS,
        .lead_seat          = NORTH,
        .tricks_taken_by_ns = 5,
        .best_plays         = Cards({"A♦", "K♣"}),
    },
    {
        // https://en.wikipedia.org/wiki/Trump_squeeze
        .name               = "double_trump_squeeze",
        .west               = Cards("♠ Q42 ♥ -  ♦ K7 ♣ -"),
        .north              = Cards("♠ T73 ♥ AT ♦ -  ♣ -"),
        .east               = Cards("♠ J96 ♥ -  ♦ J9 ♣ -"),
        .south              = Cards("♠ AK8 ♥ -  ♦ QT ♣ -"),
        .trump_suit         = HEARTS,
        .lead_seat          = NORTH,
        .tricks_taken_by_ns = 5,
        .best_plays         = Cards({"A♥", "T♥"}),
    },
    {
        // https://en.wikipedia.org/wiki/Compound_squeeze
        .name               = "compound_squeeze",
        .west               = Cards("♠ -  ♥ Q76 ♦ Q95 ♣ -"),
        .north              = Cards("♠ -  ♥ AJ  ♦ K86 ♣ 7"),
        .east               = Cards("♠ -  ♥ KT  ♦ JT2 ♣ K"),
        .south              = Cards("♠ 53 ♥ 4   ♦ A7  ♣ J"),
        .trump_suit         = SPADES,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 6,
        .best_plays         = Cards({"5♠", "3♠"}),
    },
    {
        // https://en.wikipedia.org/wiki/Saturated_squeeze
        .name               = "saturated_squeeze",
        .west               = Cards("♠ A ♥ QT8 ♦ -   ♣ QJT"),
        .north              = Cards("♠ 2 ♥ -   ♦ AK9 ♣ A42"),
        .east               = Cards("♠ K ♥ J97 ♦ QJT ♣ -"),
        .south              = Cards("♠ - ♥ AK2 ♦ 2   ♣ K53"),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = NORTH,
        .tricks_taken_by_ns = 7,
        .best_plays         = Cards({"A♦", "K♦"}),
    },
    {
        // https://en.wikipedia.org/wiki/Trump_coup
        .name               = "trump_coup_endplay",
        .west               = Cards("♠ JT87  ♥ - ♦ J8 ♣ -"),
        .north              = Cards("♠ 95432 ♥ T ♦ -  ♣ -"),
        .east               = Cards("♠ Q6    ♥ J ♦ -  ♣ 983"),
        .south              = Cards("♠ AK    ♥ - ♦ -  ♣ J765"),
        .trump_suit         = CLUBS,
        .lead_seat          = NORTH,
        .tricks_taken_by_ns = 5,
        .best_plays         = Cards({"T♥"}),
    },
    {
        // https://en.wikipedia.org/wiki/Uppercut_(bridge)
        .name               = "uppercut",
        .west               = Cards("♠ Q6    ♥ 6 ♦ 54 ♣ 8"),
        .north              = Cards("♠ T975  ♥ A ♦ 6  ♣ -"),
        .east               = Cards("♠ J2    ♥ - ♦ 32 ♣ 95"),
        .south              = Cards("♠ AK843 ♥ - ♦ A  ♣ -"),
        .trump_suit         = SPADES,
        .lead_seat          = WEST,
        .tricks_taken_by_ns = 5,
        .best_plays         = Cards({"6♥"}),
    },
    {
        // https://en.wikipedia.org/wiki/Trump_coup
        .name               = "grand_coup",
        .west               = Cards("♠ T73   ♥ J83  ♦ T9   ♣ J974"),
        .north              = Cards("♠ 8     ♥ 65   ♦ K32  ♣ KQT853"),
        .east               = Cards("♠ 9642  ♥ QT9  ♦ 8654 ♣ 6"),
        .south              = Cards("♠ AKQJ5 ♥ AK7  ♦ Q7   ♣ A2"),
        .trump_suit         = CLUBS,
        .lead_seat          = EAST,
        .tricks_taken_by_ns = 12,
        .best_plays         = Cards::all(),
    },
};

INSTANTIATE_TEST_SUITE_P(
    Solver,
    ManualTest,
    testing::ValuesIn(MANUAL_TESTS),
    [](const auto &info) { return info.param.name; }
);
