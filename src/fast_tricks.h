#pragma once

#include "game_model.h"

void estimate_fast_tricks(
    const Hands &hands,
    Seat         my_seat,
    Suit         trump_suit,
    int         &fast_tricks,
    Cards       &winners_by_rank
);
