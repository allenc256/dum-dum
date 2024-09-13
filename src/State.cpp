#include "State.h"

#include <random>

#include "Panic.h"

void print_spaces(std::ostream& os, int n);
int print_cards_in_suit(std::ostream& os, Cards c, Suit s);

std::ostream& operator<<(std::ostream& os, Seat s) {
  switch (s) {
    case Seat::WEST:
      os << "W";
      break;
    case Seat::NORTH:
      os << "N";
      break;
    case Seat::EAST:
      os << "E";
      break;
    case Seat::SOUTH:
      os << "S";
      break;
    default:
      PANIC("bad seat");
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const State& s) {
  constexpr int spacing = 15;

  for (int i = 0; i < 4; i++) {
    print_spaces(os, spacing);
    print_cards_in_suit(os, s.north(), (Suit)i);
    os << std::endl;
  }

  for (int i = 0; i < 4; i++) {
    int count = print_cards_in_suit(os, s.west(), (Suit)i);
    print_spaces(os, 2 * spacing - count);
    print_cards_in_suit(os, s.east(), (Suit)i);
    os << std::endl;
  }

  for (int i = 0; i < 4; i++) {
    print_spaces(os, spacing);
    print_cards_in_suit(os, s.south(), (Suit)i);
    os << std::endl;
  }

  return os;
}

static std::default_random_engine RANDOM;

State State::random() {
  int indexes[52];
  for (int i = 0; i < 52; i++) {
    indexes[i] = i;
  }
  std::shuffle(indexes, indexes + 52, RANDOM);

  Cards c[4];
  for (int i = 0; i < 13; i++) {
    for (int j = 0; j < 4; j++) {
      c[j].add(indexes[i + 13 * j]);
    }
  }

  std::uniform_int_distribution<int> uniform_dist(0, 3);
  Seat lead = (Seat)uniform_dist(RANDOM);

  return State(lead, c[0], c[1], c[2], c[3]);
}
