#include <gtest/gtest.h>

#include "game_model.h"
#include "random.h"

TEST(Random, random_game) {
  Game g = Random(123).random_game(13);

  for (int n = 0; n < 100; n++) {
    for (int i = 0; i < 4; i++) {
      for (int j = i + 1; j < 4; j++) {
        ASSERT_TRUE(g.hand((Seat)i).disjoint(g.hand((Seat)j)));
      }
    }

    int total = 0;
    for (int i = 0; i < 4; i++) {
      total += g.hand((Seat)i).count();
    }
    ASSERT_EQ(total, 52);
  }
}