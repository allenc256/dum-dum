#pragma once

#include "card_model.h"
#include "game_model.h"

#include <random>

class Random {
public:
  Random(int seed)
      : rng_(seed),
        rank_dist_(0, 12),
        suit_dist_(0, 3),
        trump_suit_dist_(0, 4),
        seat_dist_(0, 3),
        uniform_dist_(0.0f, 1.0f) {}

  float random_uniform() { return uniform_dist_(rng_); }
  Rank  random_rank() { return (Rank)rank_dist_(rng_); }
  Suit  random_suit() { return (Suit)suit_dist_(rng_); }
  Suit  random_trump_suit() { return (Suit)trump_suit_dist_(rng_); }
  Seat  random_seat() { return (Seat)seat_dist_(rng_); }

  void random_deal(Cards hands[4], int cards_per_hand) {
    int indexes[52];
    for (int i = 0; i < 52; i++) {
      indexes[i] = i;
    }
    std::shuffle(indexes, indexes + 52, rng_);

    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      hands[seat].clear();
      for (int i = 0; i < cards_per_hand; i++) {
        hands[seat].add(indexes[i + 13 * seat]);
      }
    }
  }

  Game random_game(int cards_per_hand) {
    Suit  trump_suit = random_trump_suit();
    Seat  lead_seat  = random_seat();
    Cards hands[4];
    random_deal(hands, cards_per_hand);
    return Game(trump_suit, lead_seat, hands);
  }

private:
  std::default_random_engine            rng_;
  std::uniform_int_distribution<int>    rank_dist_;
  std::uniform_int_distribution<int>    suit_dist_;
  std::uniform_int_distribution<int>    trump_suit_dist_;
  std::uniform_int_distribution<int>    seat_dist_;
  std::uniform_real_distribution<float> uniform_dist_;
};
