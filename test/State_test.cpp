#include "State.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(State, random) {
  std::default_random_engine random(123);
  State s = State::random(random);

  for (int n = 0; n < 100; n++) {
    for (int i = 0; i < 4; i++) {
      for (int j = i + 1; j < 4; j++) {
        ASSERT_TRUE(s.cards((Seat)i).disjoint(s.cards((Seat)j)));
      }
    }

    int total = 0;
    for (int i = 0; i < 4; i++) {
      total += s.cards((Seat)i).count();
    }
    ASSERT_EQ(total, 52);
  }
}
