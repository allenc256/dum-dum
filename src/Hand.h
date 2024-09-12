#pragma once

#include <cstdint>

#include "Card.h"

class Hand {
 public:
  Hand();

 private:
  uint64_t _cards;
};

inline Hand::Hand() : _cards(0) {}