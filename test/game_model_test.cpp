#include "game_model.h"
#include "test_util.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

Trick make_trick(Suit trump_suit, Seat lead, std::vector<std::string> cards) {
  Trick t;
  t.start_trick(trump_suit, lead, cards[0]);
  for (int i = 1; i < cards.size(); i++) {
    t.continue_trick(cards[i]);
  }
  return t;
}

void test_trick(Suit trump_suit, Seat lead, std::vector<std::string> cards,
                Seat expected_winner) {
  Trick t = make_trick(trump_suit, lead, cards);
  EXPECT_TRUE(t.started());
  EXPECT_TRUE(t.finished());
  EXPECT_EQ(t.winning_seat(), expected_winner);
}

TEST(Trick, ostream) {
  EXPECT_EQ(to_string(Trick()), "-");
  EXPECT_EQ(to_string(make_trick(NO_TRUMP, WEST, {"2C", "3C"})), "W:2♣ N:3♣");
}

TEST(Trick, no_trump) {
  test_trick(NO_TRUMP, WEST, {"2C", "3C", "TC", "8C"}, EAST);
}

TEST(Trick, no_trump_discard) {
  test_trick(NO_TRUMP, WEST, {"JC", "3C", "TC", "AS"}, WEST);
}

TEST(Trick, trump_no_ruff) {
  test_trick(HEARTS, WEST, {"2C", "AC", "TC", "8C"}, NORTH);
}

TEST(Trick, trump_ruff) {
  test_trick(HEARTS, WEST, {"2C", "AC", "TC", "2H"}, SOUTH);
}

TEST(Trick, trump_overruff) {
  test_trick(HEARTS, WEST, {"2C", "2H", "3H", "TC"}, EAST);
}

TEST(Trick, trump_discard) {
  test_trick(HEARTS, WEST, {"2C", "2S", "3C", "TC"}, SOUTH);
}

TEST(Trick, trump_ruff_discard) {
  test_trick(HEARTS, WEST, {"2C", "2H", "3C", "TC"}, NORTH);
}

TEST(Game, random_deal) {
  std::default_random_engine random(123);
  Game s = Game::random_deal(random, 13);

  for (int n = 0; n < 100; n++) {
    for (int i = 0; i < 4; i++) {
      for (int j = i + 1; j < 4; j++) {
        ASSERT_TRUE(s.hand((Seat)i).disjoint(s.hand((Seat)j)));
      }
    }

    int total = 0;
    for (int i = 0; i < 4; i++) {
      total += s.hand((Seat)i).count();
    }
    ASSERT_EQ(total, 52);
  }
}
