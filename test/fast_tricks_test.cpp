#include "fast_tricks.h"
#include "random.h"
#include "solver.h"

#include <gtest/gtest.h>

void test_fast_tricks(
    const char *hands_str, Suit trump_suit, int exp_fast_tricks, Cards exp_wbr
) {
  Hands hands(hands_str);
  int   fast_tricks;
  Cards winners_by_rank;
  estimate_fast_tricks(hands, WEST, trump_suit, fast_tricks, winners_by_rank);
  EXPECT_EQ(fast_tricks, exp_fast_tricks);
  EXPECT_EQ(winners_by_rank, exp_wbr);
}

TEST(fast_tricks, empty) {
  test_fast_tricks(".../.../.../...", NO_TRUMP, 0, Cards());
}

TEST(fast_tricks, end_in_hand) {
  test_fast_tricks("...AK/...QJ/...T9/...87", NO_TRUMP, 2, Cards("...AK"));
}

TEST(fast_tricks, end_in_pa) {
  test_fast_tricks("...32/...54/...AK/...76", NO_TRUMP, 2, Cards("...AK"));
}

TEST(fast_tricks, opp_ruffs) {
  test_fast_tricks("...AK/32.../...32/...54", SPADES, 0, Cards());
  test_fast_tricks("...AK/32.../...32/...54", NO_TRUMP, 2, Cards("...AK"));
  test_fast_tricks("...AK/...32/...54/32...", SPADES, 0, Cards());
  test_fast_tricks("...AK/...32/...54/32...", NO_TRUMP, 2, Cards("...AK"));
}

TEST(fast_tricks, length_tricks_end_in_hand) {
  test_fast_tricks(
      "...AK32/32...QJ/7654.../AKQJ...", NO_TRUMP, 4, Cards("...AK")
  );
}

TEST(fast_tricks, length_tricks_end_in_pa) {
  test_fast_tricks(
      "765...4/32...QJ/...AK32/AKQJ...", NO_TRUMP, 4, Cards("...AK")
  );
}

TEST(fast_tricks, transfer) {
  test_fast_tricks("KQ2...2/.../A3...AK/...", NO_TRUMP, 4, Cards("AK...AK"));
}

TEST(fast_tricks, length_tricks_discards) {
  test_fast_tricks("AKQ...2/.../2...AKQ/...", NO_TRUMP, 4, Cards("AKQ...A"));
  test_fast_tricks("...AKQ/.../32...2/...", SPADES, 1, Cards("...AKQ"));
}

TEST(fast_tricks, random) {
  for (int seed = 0; seed < 100; seed++) {
    Random random(seed);
    Game   game = random.random_game(6);
    Solver solver(game);

    solver.enable_fast_tricks(false);

    auto  result = solver.solve();
    int   fast_tricks;
    Cards winners_by_rank;

    estimate_fast_tricks(
        game.hands(),
        game.next_seat(),
        game.trump_suit(),
        fast_tricks,
        winners_by_rank
    );

    if (game.next_seat() == NORTH || game.next_seat() == SOUTH) {
      ASSERT_LE(fast_tricks, result.tricks_taken_by_ns);
    } else {
      ASSERT_LE(fast_tricks, result.tricks_taken_by_ew);
    }
  }
}
