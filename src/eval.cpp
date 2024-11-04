#include "eval.h"

#include <cassert>
#include <iostream>

[[maybe_unused]] const float LINEAR0_WEIGHT[4212 * 4] = {
#include "weights/linear0_weight.txt"
};

[[maybe_unused]] const float LINEAR0_BIAS[4] = {
#include "weights/linear0_bias.txt"
};

[[maybe_unused]] const float LINEAR1_WEIGHT[16 * 16] = {
#include "weights/linear1_weight.txt"
};

[[maybe_unused]] const float LINEAR1_BIAS[16 * 16] = {
#include "weights/linear1_bias.txt"
};

[[maybe_unused]] const float LINEAR2_WEIGHT[16] = {
#include "weights/linear2_weight.txt"
};

[[maybe_unused]] const float LINEAR2_BIAS[1] = {
#include "weights/linear2_bias.txt"
};

void eval_layer0([[maybe_unused]] int features[32], float output[16]) {
  for (int i = 0; i < 16; i += 4) {
    output[i]     = LINEAR0_BIAS[0];
    output[i + 1] = LINEAR0_BIAS[1];
    output[i + 2] = LINEAR0_BIAS[2];
    output[i + 3] = LINEAR0_BIAS[3];
  }

  for (int i = 0; i < 32; i++) {
    int feature = features[i];
    int s_off   = (feature & 0b11) * 4;
    int w_off   = ((uint32_t)feature & ~(uint32_t)0b11);
    output[s_off] += LINEAR0_WEIGHT[w_off];
    output[s_off + 1] += LINEAR0_WEIGHT[w_off + 1];
    output[s_off + 2] += LINEAR0_WEIGHT[w_off + 2];
    output[s_off + 3] += LINEAR0_WEIGHT[w_off + 3];
  }
}
