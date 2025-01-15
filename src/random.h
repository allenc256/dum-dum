#pragma once

#include <random>

#include "card_model.h"
#include "game_model.h"

class Random {
public:
  Random(int seed);

  float random_uniform();
  Rank  random_rank();
  Suit  random_suit();
  Suit  random_trump_suit();
  Seat  random_seat();
  Hands random_deal(int cards_per_hand);
  Game  random_game(int cards_per_hand);

private:
  std::mt19937_64                       rng_;
  std::uniform_int_distribution<int>    rank_dist_;
  std::uniform_int_distribution<int>    suit_dist_;
  std::uniform_int_distribution<int>    trump_suit_dist_;
  std::uniform_int_distribution<int>    seat_dist_;
  std::uniform_real_distribution<float> uniform_dist_;
};
