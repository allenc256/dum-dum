#pragma once

void eval_layer0_naive(const int features[32], float output[16]);
void eval_layer0_arm_neon(const int features[32], float output[16]);
void eval_layer0_arm_neon_fp16(const int features[32], float output[16]);
