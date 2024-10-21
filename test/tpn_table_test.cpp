#include "random.h"
#include "tpn_table.h"

#include <gtest/gtest.h>

TEST(SeatShape, empty) {
  SeatShape s;
  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    EXPECT_EQ(s.card_count(suit), 0);
  }
}

TEST(SeatShape, non_empty) {
  SeatShape s(Cards("♠ 8 ♥ 65 ♦ K32 ♣ KQT853"));
  EXPECT_EQ(s.card_count(SPADES), 1);
  EXPECT_EQ(s.card_count(HEARTS), 2);
  EXPECT_EQ(s.card_count(DIAMONDS), 3);
  EXPECT_EQ(s.card_count(CLUBS), 6);
}

void test_abs_suit_state(int seed) {
  Random random(seed);
  Rank   rank_cutoff = random.random_rank();
  Hands  hands       = random.random_deal(10);

  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    AbsSuitState state(rank_cutoff, suit, hands);
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      Cards cards      = hands.hand(seat).intersect(suit);
      Cards low_cards  = cards.low_cards(rank_cutoff);
      Cards high_cards = cards.subtract(low_cards);
      EXPECT_EQ(state.high_cards(seat, suit), high_cards);
      EXPECT_EQ(state.low_cards(seat), low_cards.count());
    }
  }
}

TEST(AbsSuitState, random_deal) {
  for (int seed = 0; seed < 500; seed++) {
    test_abs_suit_state(seed);
  }
}

TEST(AbsState, matches) {
  Random   random = Random(123);
  Game     game1  = random.random_game(10);
  Game     game2  = random.random_game(10);
  AbsLevel level  = AbsLevel({RANK_8, RANK_9, TEN, JACK});
  AbsState state1(level, game1);
  AbsState state2(level, game2);
  EXPECT_TRUE(state1.matches(game1));
  EXPECT_TRUE(state2.matches(game2));
  EXPECT_FALSE(state1.matches(game2));
  EXPECT_FALSE(state2.matches(game1));
}
