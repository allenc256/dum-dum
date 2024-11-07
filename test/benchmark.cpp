#include "eval.h"

#include <benchmark/benchmark.h>
#include <iostream>

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

static void BM_eval_layer0_naive(benchmark::State &state) {
  alignas(16) float out[16];
  for (auto _ : state) {
    eval_layer0_naive(GAME, out);
  }
}

static void BM_eval_layer1_naive(benchmark::State &state) {
  alignas(16) float out[16];
  for (auto _ : state) {
    eval_layer1_naive(GAME.tricks_left(), OUT0, out);
  }
}

static void BM_eval_mlp_network(benchmark::State &state) {
  for (auto _ : state) {
    eval_mlp_network(GAME);
  }
}

BENCHMARK(BM_eval_layer0_naive);
BENCHMARK(BM_eval_layer1_naive);
BENCHMARK(BM_eval_mlp_network);

BENCHMARK_MAIN();
