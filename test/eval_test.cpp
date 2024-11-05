#include "eval.h"

#include <gtest/gtest.h>

static const int FEATURES[32] = {
    6488, 6480, 6460, 6516, 6596, 6580, 6576, 6652, 6640, 6624, 7741,
    7733, 7797, 7789, 7829, 7885, 7877, 8366, 8358, 8350, 8406, 8390,
    8450, 8446, 8438, 8526, 8518, 1975, 1963, 1959, 2023, 2071,
};

static const float OUTPUT[16] = {
    0.2025102,
    1.0128818,
    -0.14067471,
    0.14914006,
    0.88365465,
    0.9768978,
    -0.16632055,
    0.38320038,
    0.34305304,
    0.83159864,
    -0.2803467,
    0.28927544,
    0.42370218,
    0.61149293,
    0.19096339,
    0.44793922,
};

TEST(Eval, eval_layer0_naive) {
  alignas(16) float output[16];
  eval_layer0_naive(FEATURES, output);
  for (int i = 0; i < 16; i++) {
    EXPECT_FLOAT_EQ(OUTPUT[i], output[i]);
  }
}

TEST(Eval, eval_layer0_arm_neon) {
  alignas(16) float output[16];
  eval_layer0_arm_neon(FEATURES, output);
  for (int i = 0; i < 16; i++) {
    EXPECT_FLOAT_EQ(OUTPUT[i], output[i]);
  }
}

TEST(Eval, eval_layer0_arm_neon_fp16) {
  alignas(16) float output[16];
  eval_layer0_arm_neon_fp16(FEATURES, output);
  for (int i = 0; i < 16; i++) {
    EXPECT_NEAR(OUTPUT[i], output[i], 1e-3);
  }
}
