#include "eval.h"

#include <benchmark/benchmark.h>
#include <iostream>

const int features[32] = {6488, 6480, 6460, 6516, 6596, 6580, 6576, 6652,
                          6640, 6624, 7741, 7733, 7797, 7789, 7829, 7885,
                          7877, 8366, 8358, 8350, 8406, 8390, 8450, 8446,
                          8438, 8526, 8518, 1975, 1963, 1959, 2023, 2071};

static void BM_eval_layer0_naive(benchmark::State &state) {
  alignas(16) float output[16];
  for (auto _ : state) {
    eval_layer0_naive(features, output);
  }
}

static void BM_eval_layer0_arm_neon(benchmark::State &state) {
  alignas(16) float output[16];
  for (auto _ : state) {
    eval_layer0_arm_neon(features, output);
  }
}

static void BM_eval_layer0_arm_neon_fp16(benchmark::State &state) {
  alignas(16) float output[16];
  for (auto _ : state) {
    eval_layer0_arm_neon_fp16(features, output);
  }
}

BENCHMARK(BM_eval_layer0_naive);
BENCHMARK(BM_eval_layer0_arm_neon);
BENCHMARK(BM_eval_layer0_arm_neon_fp16);

BENCHMARK_MAIN();
