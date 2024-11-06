#include "eval.h"

#include <gtest/gtest.h>

static const int FEATURES[32] = {
    6488, 6480, 6460, 6516, 6596, 6580, 6576, 6652, 6640, 6624, 7741,
    7733, 7797, 7789, 7829, 7885, 7877, 8366, 8358, 8350, 8406, 8390,
    8450, 8446, 8438, 8526, 8518, 1975, 1963, 1959, 2023, 2071,
};

static const float OUTPUT[16] = {
    0.0,
    0.0,
    0.0,
    0.24163932,
    0.0,
    0.0,
    0.0684607,
    0.0255655,
    0.0,
    0.0,
    0.16984534,
    0.0,
    0.2826414,
    0.0,
    0.0,
    0.0
};

TEST(Eval, eval_layer0_naive) {
  alignas(16) float output[16];
  Game              game = {
      NO_TRUMP,
      WEST,
      Hands("QT5.KJ.KJ9./6.AQ.T6.AJT/K98.9.875.K/AJ7.T8.AQ.Q"),
  };
  eval_layer0_naive(game, output);
  for (int i = 0; i < 16; i++) {
    EXPECT_NEAR(OUTPUT[i], output[i], 1e-5);
  }
}
