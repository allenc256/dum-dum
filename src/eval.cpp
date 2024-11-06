#include "eval.h"

// #include <arm_neon.h>
#include <cassert>
#include <iostream>

alignas(16) const float WEIGHT_W0_FLAT[7 * 4212 * 4] = {
#include "weights/w0.txt"
};

alignas(16) const float WEIGHT_B0_FLAT[7 * 4] = {
#include "weights/b0.txt"
};

const float (*WEIGHT_W0)[4212][4] = (float (*)[4212][4])WEIGHT_W0_FLAT;
const float (*WEIGHT_B0)[4]       = (float (*)[4])WEIGHT_B0_FLAT;

constexpr int THRESHOLDS[7][2] = {
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 2},
    {1, 2},
    {1, 3},
    {1, 3},
};

void eval_layer0_naive(const Game &game, float output[16]) {
  int model_idx = game.tricks_left() - 2;

  assert(game.start_of_trick());
  assert(game.next_seat() == WEST);
  assert(game.trump_suit() == NO_TRUMP);
  assert(model_idx >= 0 && model_idx < 7);

  const float(*w0)[4] = WEIGHT_W0[model_idx];
  const float *b0     = WEIGHT_B0[model_idx];
  const int   *thresh = THRESHOLDS[model_idx];

  auto &hands = game.normalized_state().normalized_hands;
  int   shape[4];

  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    int n = hands.hand(seat).count();
    if (n <= thresh[0]) {
      shape[seat] = 0;
    } else if (n <= thresh[1]) {
      shape[seat] = 1;
    } else {
      shape[seat] = 2;
    }
  }

  for (int i = 0; i < 16; i += 4) {
    output[i]     = b0[0];
    output[i + 1] = b0[1];
    output[i + 2] = b0[2];
    output[i + 3] = b0[3];
  }

  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    int shape[4];
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      int n = hands.hand(seat).intersect(suit).count();
      if (n <= thresh[0]) {
        shape[seat] = 0;
      } else if (n <= thresh[1]) {
        shape[seat] = 1;
      } else {
        shape[seat] = 2;
      }
    }

    int output_off = (3 - suit) * 4;
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      Cards cards = hands.hand(seat).intersect(suit);
      for (auto it = cards.iter_highest(); it.valid();
           it      = cards.iter_lower(it)) {
        Card card   = it.card();
        int  w0_off = shape[0];
        w0_off      = w0_off * 3 + shape[1];
        w0_off      = w0_off * 3 + shape[2];
        w0_off      = w0_off * 3 + shape[3];
        w0_off      = w0_off * 4 + seat;
        w0_off      = w0_off * 13 + card.rank();
        output[output_off] += w0[w0_off][0];
        output[output_off + 1] += w0[w0_off][1];
        output[output_off + 2] += w0[w0_off][2];
        output[output_off + 3] += w0[w0_off][3];
      }
    }
  }

  for (int i = 0; i < 16; i++) {
    output[i] = std::max(0.0f, output[i]);
  }
}
