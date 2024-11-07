#include "eval.h"

#include <gtest/gtest.h>

static const Game GAME = {
    NO_TRUMP,
    WEST,
    Hands("QT5.KJ.KJ9./6.AQ.T6.AJT/K98.9.875.K/AJ7.T8.AQ.Q"),
};

static const float OUT0[16] = {
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
    0.0,
};

static const float OUT1[16] = {
    0.20686932,
    0.14571789,
    0.3757962,
    0.47782296,
    0.43490952,
    0.33066976,
    0.2979607,
    0.1302946,
    0.05528658,
    0.17764455,
    0.05846167,
    0.58477515,
    0.1175013,
    0.08126146,
    0.12586138,
    0.20396715,
};

static const float OUT2 = 0.86754996;

TEST(Eval, eval_layer0_naive) {
  alignas(16) float out[16];
  eval_layer0_naive(GAME, out);
  for (int i = 0; i < 16; i++) {
    EXPECT_NEAR(OUT0[i], out[i], 1e-5);
  }
}

TEST(Eval, eval_layer1_naive) {
  alignas(16) float out[16];
  eval_layer1_naive(GAME.tricks_left(), OUT0, out);
  for (int i = 0; i < 16; i++) {
    EXPECT_NEAR(OUT1[i], out[i], 1e-5);
  }
}

TEST(Eval, eval_layer2_naive) {
  float out = eval_layer2_naive(GAME.tricks_left(), OUT1);
  EXPECT_NEAR(OUT2, out, 1e-5);
}

TEST(Eval, eval_mlp_network) {
  float out = eval_mlp_network(GAME);
  EXPECT_NEAR(OUT2 * (float)GAME.tricks_left(), out, 1e-5);
}
