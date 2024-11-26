#include "random.h"
#include "solver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(PlayOrder, empty) {
  PlayOrder order;
  for (Card c : order) {
    FAIL();
  }
}

TEST(PlayOrder, order_cards) {
  PlayOrder order;
  order.append_plays(Cards({"2C", "5C", "AC"}), PlayOrder::LOW_TO_HIGH);
  order.append_plays(Cards({"5C", "6C", "9C"}), PlayOrder::HIGH_TO_LOW);

  std::array<Card, 5> expected = {"2C", "5C", "AC", "9C", "6C"};
  int                 n        = 0;
  for (Card c : order) {
    ASSERT_LT(n, expected.size());
    ASSERT_EQ(c, expected[n]);
    n++;
  }
}

constexpr int DEAL_SIZE = 4;

void validate_solver(Solver &s1) {
  Solver s2 = Solver(s1.game());
  s2.enable_all_optimizations(false);
  auto r1 = s1.solve();
  auto r2 = s2.solve();
  ASSERT_EQ(r1.tricks_taken_by_ns, r2.tricks_taken_by_ns);
}

TEST(Solver, ab_pruning) {
  for (int seed = 0; seed < 100; seed++) {
    Game   g = Random(seed).random_game(DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_ab_pruning(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "seed " << seed);
      validate_solver(s);
    });
  }
}

TEST(Solver, tpn_table) {
  for (int seed = 0; seed < 100; seed++) {
    Game   g = Random(seed).random_game(DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_tpn_table(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "seed " << seed);
      validate_solver(s);
    });
  }
}

TEST(Solver, move_ordering) {
  for (int seed = 0; seed < 100; seed++) {
    Game   g = Random(seed).random_game(DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_move_ordering(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "seed " << seed);
      validate_solver(s);
    });
  }
}

TEST(Solver, fast_tricks) {
  for (int seed = 0; seed < 100; seed++) {
    Game   g = Random(seed).random_game(DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(false);
    s.enable_fast_tricks(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "seed " << seed);
      validate_solver(s);
    });
  }
}

TEST(Solver, all_optimizations) {
  for (int seed = 0; seed < 100; seed++) {
    Game   g = Random(seed).random_game(DEAL_SIZE);
    Solver s = Solver(g);
    s.enable_all_optimizations(true);
    ASSERT_NO_FATAL_FAILURE({
      SCOPED_TRACE(::testing::Message() << "seed " << seed);
      validate_solver(s);
    });
  }
}

struct ManualTestCase {
  const char *name;
  Hands       hands;
  Suit        trump_suit;
  Seat        lead_seat;
  int         tricks_taken_by_ns;
  Cards       best_plays;
};

class ManualTest : public testing::TestWithParam<ManualTestCase> {};

TEST_P(ManualTest, manual_test) {
  const ManualTestCase &p = GetParam();
  Game                  g(p.trump_suit, p.lead_seat, p.hands);
  Solver                s(g);
  auto                  r = s.solve();
  EXPECT_EQ(r.tricks_taken_by_ns, p.tricks_taken_by_ns);
}

const ManualTestCase MANUAL_TESTS[] = {
    {
        // https://en.wikipedia.org/wiki/Simple_squeeze
        .name               = "simple_squeeze",
        .hands              = Hands("KQ.A../AJ.K../.QJT../4.2..A"),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 3,
        .best_plays         = Cards({"A♣"}),
    },
    {
        // https://en.wikipedia.org/wiki/Simple_squeeze
        .name               = "split_two_card_threat_squeeze",
        .hands              = Hands("KQ.A../A3.K../.QJT../J2...A"),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 3,
        .best_plays         = Cards({"A♣"}),
    },
    {
        // https://en.wikipedia.org/wiki/Simple_squeeze
        .name               = "criss_cross_squeeze",
        .hands              = Hands("...6543/A.Q2..2/K3.K3../Q2.A..A"),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 4,
    },
    {
        // https://en.wikipedia.org/wiki/Simple_squeeze
        .name               = "vienna_coup",
        .hands              = Hands("...5432/AJ.A.2./KQ.K3../2.Q2.A."),
        .trump_suit         = NO_TRUMP,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 4,
    },
    {
        // https://en.wikipedia.org/wiki/Trump_squeeze
        .name               = "trump_squeeze",
        .hands              = Hands("..65432./A..A.KT7/Q9...J98/T83.2..3"),
        .trump_suit         = HEARTS,
        .lead_seat          = NORTH,
        .tricks_taken_by_ns = 5,
    },
    {
        // https://en.wikipedia.org/wiki/Trump_squeeze
        .name               = "double_trump_squeeze",
        .hands              = Hands("Q42..K7./T73.AT../J96..J9./AK8..QT."),
        .trump_suit         = HEARTS,
        .lead_seat          = NORTH,
        .tricks_taken_by_ns = 5,
    },
    {
        // https://en.wikipedia.org/wiki/Compound_squeeze
        .name               = "compound_squeeze",
        .hands              = Hands(".Q76.Q95./.AJ.K86.7/.KT.JT2.K/53.4.A7.J"),
        .trump_suit         = SPADES,
        .lead_seat          = SOUTH,
        .tricks_taken_by_ns = 6,
    },
    {
        // https://en.wikipedia.org/wiki/Saturated_squeeze
        .name       = "saturated_squeeze",
        .hands      = Hands("A.QT8..QJT/2..AK9.A42/K.J97.QJT./.AK2.2.K53"),
        .trump_suit = NO_TRUMP,
        .lead_seat  = NORTH,
        .tricks_taken_by_ns = 7,
    },
    {
        // https://en.wikipedia.org/wiki/Trump_coup
        .name               = "trump_coup_endplay",
        .hands              = Hands("JT87..J8./95432.T../Q6.J..983/AK...J765"),
        .trump_suit         = CLUBS,
        .lead_seat          = NORTH,
        .tricks_taken_by_ns = 5,
    },
    {
        // https://en.wikipedia.org/wiki/Uppercut_(bridge)
        .name               = "uppercut",
        .hands              = Hands("Q6.6.54.8/T975.A.6./J2..32.95/AK843..A."),
        .trump_suit         = SPADES,
        .lead_seat          = WEST,
        .tricks_taken_by_ns = 5,
    },
    {
        // https://en.wikipedia.org/wiki/Trump_coup
        .name  = "grand_coup",
        .hands = Hands(
            "T73.J83.T9.J974/8.65.K32.KQT853/9642.QT9.8654.6/AKQJ5.AK7.Q7.A2"
        ),
        .trump_suit         = CLUBS,
        .lead_seat          = EAST,
        .tricks_taken_by_ns = 12,
    },
};

INSTANTIATE_TEST_SUITE_P(
    Solver,
    ManualTest,
    testing::ValuesIn(MANUAL_TESTS),
    [](const auto &info) { return info.param.name; }
);
