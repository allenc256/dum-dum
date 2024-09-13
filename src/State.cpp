#include "State.h"

#include <random>

#include "Panic.h"

static void print_spaces(std::ostream &os, int n) {
  for (int i = 0; i < n; i++) {
    os << " ";
  }
}

int print_cards_in_suit(std::ostream &os, Cards c, Suit s);

const char *SEAT_STR = "WNES";

std::ostream &operator<<(std::ostream &os, Seat s) {
  if (s < 0 || s >= 4) {
    throw std::runtime_error("bad seat");
  }
  os << SEAT_STR[s];
  return os;
}

std::ostream &operator<<(std::ostream &os, const State &s) {
  constexpr int spacing = 15;

  for (int i = 0; i < 4; i++) {
    print_spaces(os, spacing);
    print_cards_in_suit(os, s.cards(NORTH), (Suit)i);
    os << std::endl;
  }

  for (int i = 0; i < 4; i++) {
    int count = print_cards_in_suit(os, s.cards(WEST), (Suit)i);
    print_spaces(os, 2 * spacing - count);
    print_cards_in_suit(os, s.cards(EAST), (Suit)i);
    os << std::endl;
  }

  for (int i = 0; i < 4; i++) {
    print_spaces(os, spacing);
    print_cards_in_suit(os, s.cards(SOUTH), (Suit)i);
    os << std::endl;
  }

  return os;
}

State State::random(std::default_random_engine &random) {
  int indexes[52];
  for (int i = 0; i < 52; i++) {
    indexes[i] = i;
  }
  std::shuffle(indexes, indexes + 52, random);

  Cards c[4];
  for (int i = 0; i < 13; i++) {
    for (int j = 0; j < 4; j++) {
      c[j].add(indexes[i + 13 * j]);
    }
  }

  std::uniform_int_distribution<int> uniform_dist(0, 3);
  Seat lead = (Seat)uniform_dist(random);

  return State(lead, c);
}
