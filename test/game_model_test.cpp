#include "game_model.h"
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

template <class T> std::string to_string(T x) {
  std::ostringstream os;
  os << x;
  return os.str();
}

Trick make_trick(Seat lead, std::vector<Card> cards) {
  Trick t;
  t.play_start(lead, cards[0]);
  for (int i = 1; i < cards.size(); i++) {
    t.play_continue(cards[i]);
  }
  return t;
}

TEST(Trick, ostream) {
  EXPECT_EQ(to_string(Trick()), "-");
  EXPECT_EQ(
      to_string(make_trick(WEST, {Card(RANK_2, CLUBS), Card(RANK_3, CLUBS)})),
      "W:2♣ N:3♣");
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
