#pragma once

#include "card_model.h"
#include "game_model.h"

class PlayOrder {
public:
  static constexpr bool LOW_TO_HIGH = true;
  static constexpr bool HIGH_TO_LOW = false;

  PlayOrder() : card_count_(0) {}

  const Card *begin() const { return cards_; }
  const Card *end() const { return cards_ + card_count_; }

  void append_plays(Cards cards, bool low_to_high);
  void append_play(Card card);

private:
  friend class Iter;

  Cards  all_cards_;
  Card   cards_[13];
  int8_t card_count_;
};

void order_plays(const Game &game, PlayOrder &order);
