#include "eval.h"

#include <cassert>
#include <cmath>
#include <iostream>

alignas(16) const float WEIGHT_W0_FLAT[7 * 4212 * 4] = {
#include "weights/w0.txt"
};

alignas(16) const float WEIGHT_B0_FLAT[7 * 4] = {
#include "weights/b0.txt"
};

alignas(16) const float WEIGHT_W1_FLAT[7 * 16 * 16] = {
#include "weights/w1.txt"
};

alignas(16) const float WEIGHT_B1_FLAT[7 * 16] = {
#include "weights/b1.txt"
};

alignas(16) const float WEIGHT_W2_FLAT[7 * 16] = {
#include "weights/w2.txt"
};

alignas(16) const float WEIGHT_B2[7] = {
#include "weights/b2.txt"
};

const float (*WEIGHT_W0)[4212][4] = (float (*)[4212][4])WEIGHT_W0_FLAT;
const float (*WEIGHT_B0)[4]       = (float (*)[4])WEIGHT_B0_FLAT;
const float (*WEIGHT_W1)[16][16]  = (float (*)[16][16])WEIGHT_W1_FLAT;
const float (*WEIGHT_B1)[16]      = (float (*)[16])WEIGHT_B1_FLAT;
const float (*WEIGHT_W2)[16]      = (float (*)[16])WEIGHT_W2_FLAT;

constexpr int THRESHOLDS[7][2] = {
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 2},
    {1, 2},
    {1, 3},
    {1, 3},
};

void eval_layer0_naive(const Game &game, float out[16]) {
  assert(game.start_of_trick());
  assert(game.trump_suit() == NO_TRUMP);
  assert(game.tricks_left() >= 2 && game.tricks_left() <= 8);

  int model_idx       = game.tricks_left() - 2;
  const float(*w0)[4] = WEIGHT_W0[model_idx];
  const float *b0     = WEIGHT_B0[model_idx];
  const int   *t0     = THRESHOLDS[model_idx];

  auto &hands = game.normalized_state().normalized_hands;
  int   shape[4];

  for (int seat = 0; seat < 4; seat++) {
    int n = hands.hand(game.next_seat(seat)).count();
    if (n <= t0[0]) {
      shape[seat] = 0;
    } else if (n <= t0[1]) {
      shape[seat] = 1;
    } else {
      shape[seat] = 2;
    }
  }

  for (int i = 0; i < 16; i += 4) {
    out[i]     = b0[0];
    out[i + 1] = b0[1];
    out[i + 2] = b0[2];
    out[i + 3] = b0[3];
  }

  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    int shape[4];
    for (int seat = 0; seat < 4; seat++) {
      int n = hands.hand(game.next_seat(seat)).intersect(suit).count();
      if (n <= t0[0]) {
        shape[seat] = 0;
      } else if (n <= t0[1]) {
        shape[seat] = 1;
      } else {
        shape[seat] = 2;
      }
    }

    int output_off = (3 - suit) * 4;
    for (int seat = 0; seat < 4; seat++) {
      Cards cards = hands.hand(game.next_seat(seat)).intersect(suit);
      for (auto it = cards.iter_highest(); it.valid();
           it      = cards.iter_lower(it)) {
        Card card   = it.card();
        int  w0_off = shape[0];
        w0_off      = w0_off * 3 + shape[1];
        w0_off      = w0_off * 3 + shape[2];
        w0_off      = w0_off * 3 + shape[3];
        w0_off      = w0_off * 4 + seat;
        w0_off      = w0_off * 13 + card.rank();
        out[output_off] += w0[w0_off][0];
        out[output_off + 1] += w0[w0_off][1];
        out[output_off + 2] += w0[w0_off][2];
        out[output_off + 3] += w0[w0_off][3];
      }
    }
  }

  for (int i = 0; i < 16; i++) {
    out[i] = std::max(0.0f, out[i]);
  }
}

void eval_layer1_naive(int tricks_left, const float in[16], float out[16]) {
  assert(tricks_left >= 2 && tricks_left <= 8);

  int model_idx        = tricks_left - 2;
  const float(*w1)[16] = WEIGHT_W1[model_idx];
  const float *b1      = WEIGHT_B1[model_idx];

  for (int i = 0; i < 16; i++) {
    out[i] = b1[i];
  }

  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      out[i] += w1[i][j] * in[j];
    }
  }

  for (int i = 0; i < 16; i++) {
    out[i] = std::max(0.0f, out[i]);
  }
}

float eval_layer2_naive(int tricks_left, const float in[16]) {
  assert(tricks_left >= 2 && tricks_left <= 8);

  int          model_idx = tricks_left - 2;
  const float *w2        = WEIGHT_W2[model_idx];
  const float  b2        = WEIGHT_B2[model_idx];

  float out = b2;

  for (int i = 0; i < 16; i++) {
    out += w2[i] * in[i];
  }

  return 1.0f / (1.0f + expf(-out));
}

float eval_mlp_network(const Game &game) {
  float out0[16];
  float out1[16];
  int   tricks_left = game.tricks_left();
  float out;

  eval_layer0_naive(game, out0);
  eval_layer1_naive(tricks_left, out0, out1);
  out = eval_layer2_naive(game.tricks_left(), out1);
  if (game.next_seat() == NORTH || game.next_seat() == SOUTH) {
    out = 1.0f - out;
  }
  out *= (float)game.tricks_left();
  out += (float)game.tricks_taken_by_ns();

  return out;
}
