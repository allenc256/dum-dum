#include "eval.h"

#include <arm_neon.h>
#include <cassert>
#include <iostream>

[[maybe_unused]] alignas(16) const float LINEAR0_WEIGHT[4212 * 4] = {
#include "weights/linear0_w.txt"
};

[[maybe_unused]] alignas(16) const float LINEAR0_BIAS[4] = {
#include "weights/linear0_b.txt"
};

[[maybe_unused]] alignas(16) const float LINEAR1_WEIGHT[16 * 16] = {
#include "weights/linear1_w.txt"
};

[[maybe_unused]] alignas(16) const float LINEAR1_BIAS[16 * 16] = {
#include "weights/linear1_b.txt"
};

[[maybe_unused]] alignas(16) const float LINEAR2_WEIGHT[16] = {
#include "weights/linear2_w.txt"
};

[[maybe_unused]] alignas(16) const float LINEAR2_BIAS[1] = {
#include "weights/linear2_b.txt"
};

[[maybe_unused]] alignas(16) const float16_t LINEAR0_WEIGHT_FP16[4212 * 4] = {
#include "weights/linear0_w_fp16.txt"
};

[[maybe_unused]] alignas(16) const float16_t LINEAR0_BIAS_FP16[4212 * 4] = {
#include "weights/linear0_b_fp16.txt"
};

void eval_layer0_naive(const int features[32], float output[16]) {
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

void eval_layer0_arm_neon(const int features[32], float output[16]) {
  float32x4_t o0 = vld1q_f32(LINEAR0_BIAS);
  float32x4_t o1 = o0;
  float32x4_t o2 = o0;
  float32x4_t o3 = o0;
  uint32x4_t  s0 = vdupq_n_u32(0);
  uint32x4_t  s1 = vdupq_n_u32(1);
  uint32x4_t  s2 = vdupq_n_u32(2);
  uint32x4_t  s3 = vdupq_n_u32(3);

  for (int i = 0; i < 32; i++) {
    uint32_t    f  = features[i];
    uint32x4_t  s  = vdupq_n_u32(f & 0b11);
    uint32x4_t  m0 = vceqq_u32(s, s0);
    uint32x4_t  m1 = vceqq_u32(s, s1);
    uint32x4_t  m2 = vceqq_u32(s, s2);
    uint32x4_t  m3 = vceqq_u32(s, s3);
    float32x4_t w  = vld1q_f32(LINEAR0_WEIGHT + (f & ~0b11));
    float32x4_t w0 =
        vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(w), m0));
    float32x4_t w1 =
        vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(w), m1));
    float32x4_t w2 =
        vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(w), m2));
    float32x4_t w3 =
        vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(w), m3));
    o0 = vaddq_f32(o0, w0);
    o1 = vaddq_f32(o1, w1);
    o2 = vaddq_f32(o2, w2);
    o3 = vaddq_f32(o3, w3);
  }

  vst1q_f32(output, o0);
  vst1q_f32(output + 4, o1);
  vst1q_f32(output + 8, o2);
  vst1q_f32(output + 12, o3);
}

void eval_layer0_arm_neon_fp16(const int features[32], float output[16]) {
  float16x4_t o  = vld1_f16(LINEAR0_BIAS_FP16);
  float16x8_t o0 = vcombine_f16(o, o);
  float16x8_t o1 = o0;
  uint16x8_t  s0 = vcombine_u16(vdup_n_u16(0), vdup_n_u16(1));
  uint16x8_t  s1 = vcombine_u16(vdup_n_u16(2), vdup_n_u16(3));

  for (int i = 0; i < 32; i++) {
    uint32_t    f  = features[i];
    uint16x8_t  s  = vdupq_n_u16(f & 0b11);
    uint16x8_t  m0 = vceqq_u16(s, s0);
    uint16x8_t  m1 = vceqq_u16(s, s1);
    float16x4_t wh = vld1_f16(LINEAR0_WEIGHT_FP16 + (f & ~0b11));
    float16x8_t w  = vcombine_f16(wh, wh);
    float16x8_t w0 =
        vreinterpretq_f16_u16(vandq_u16(vreinterpretq_u16_f16(w), m0));
    float32x4_t w1 =
        vreinterpretq_f16_u16(vandq_u16(vreinterpretq_u16_f16(w), m1));
    o0 = vaddq_f16(o0, w0);
    o1 = vaddq_f16(o1, w1);
  }

  vst1q_f32(output, vcvt_f32_f16(vget_low_f16(o0)));
  vst1q_f32(output + 4, vcvt_f32_f16(vget_high_f16(o0)));
  vst1q_f32(output + 8, vcvt_f32_f16(vget_low_f16(o1)));
  vst1q_f32(output + 12, vcvt_f32_f16(vget_high_f16(o1)));
}
