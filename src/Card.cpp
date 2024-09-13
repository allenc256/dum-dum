#include "Card.h"

#include <algorithm>
#include <iostream>
#include <random>

#include "Panic.h"

Rank ParseRank(char ch) {
  if (ch >= '2' && ch <= '9') {
    return static_cast<Rank>(ch - '2');
  }
  switch (ch) {
    case 'T':
      return Rank::TEN;
    case 'J':
      return Rank::JACK;
    case 'Q':
      return Rank::QUEEN;
    case 'K':
      return Rank::KING;
    case 'A':
      return Rank::ACE;
    default:
      PANIC("bad rank");
  }
}

Suit ParseSuit(std::string_view utf8_str) {
  if (utf8_str == "C" || utf8_str == "♣") {
    return Suit::CLUBS;
  } else if (utf8_str == "D" || utf8_str == "♦") {
    return Suit::DIAMONDS;
  } else if (utf8_str == "H" || utf8_str == "♥") {
    return Suit::HEARTS;
  } else if (utf8_str == "S" || utf8_str == "♠") {
    return Suit::SPADES;
  } else {
    PANIC("bad suit");
  }
}

Card::Card(std::string_view utf8_str) {
  if (utf8_str.length() < 2) {
    PANIC("bad card");
  }
  rank_ = ParseRank(utf8_str[0]);
  suit_ = ParseSuit(utf8_str.substr(1));
}

std::ostream& operator<<(std::ostream& os, Suit s) {
  switch (s) {
    case Suit::CLUBS:
      os << "♣";
      break;
    case Suit::DIAMONDS:
      os << "♦";
      break;
    case Suit::HEARTS:
      os << "♥";
      break;
    case Suit::SPADES:
      os << "♠";
      break;
    default:
      PANIC("bad suit");
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, Rank r) {
  if (r >= 0 && r < 8) {
    os << static_cast<int>(r + 2);
  } else {
    switch (r) {
      case Rank::TEN:
        os << "T";
        break;
      case Rank::JACK:
        os << "J";
        break;
      case Rank::QUEEN:
        os << "Q";
        break;
      case Rank::KING:
        os << "K";
        break;
      case Rank::ACE:
        os << "A";
        break;
      default:
        PANIC("bad rank");
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, Card c) {
  os << c.rank() << c.suit();
  return os;
}

void print_spaces(std::ostream& os, int n) {
  for (int i = 0; i < n; i++) {
    os << " ";
  }
}

int print_cards_in_suit(std::ostream& os, Cards c, Suit s) {
  c = c.intersect_suit(s);
  os << s << " ";
  int count = 0;
  for (Cards::Iter i = c.first(); i.valid(); i = c.next(i)) {
    os << i.card().rank();
    count++;
  }
  if (count <= 0) {
    os << "-";
    count++;
  }
  return count + 2;
}

std::ostream& operator<<(std::ostream& os, Cards c) {
  for (int s = 0; s < 4; s++) {
    if (s > 0) {
      os << " ";
    }
    print_cards_in_suit(os, c, (Suit)s);
  }
  return os;
}

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

std::ostream& operator<<(std::ostream& os, const GameState& s) {
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

GameState GameState::random() {
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

  return GameState(lead, c[0], c[1], c[2], c[3]);
}
