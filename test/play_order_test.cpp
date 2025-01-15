#include <array>
#include <gtest/gtest.h>

#include "play_order.h"

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
