#pragma once

#include "game_model.h"

void  eval_layer0_naive(const Game &game, float out[16]);
void  eval_layer1_naive(int tricks_left, const float in[16], float out[16]);
float eval_layer2_naive(int tricks_left, const float in[16]);

float eval_mlp_network(const Game &game);
