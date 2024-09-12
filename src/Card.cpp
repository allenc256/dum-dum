#include "Card.h"

#include <iostream>

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

std::ostream& operator<<(std::ostream& os, Cards c) {
  for (int s = 0; s < 4; s++) {
    Cards cs = c.intersect_suit((Suit)s);

    if (s > 0) {
      os << " ";
    }
    os << (Suit)s << " ";

    int count = 0;
    for (Cards::Iter i = cs.first(); i.valid(); i = cs.next(i)) {
      os << i.card().rank();
      count++;
    }

    if (count <= 0) {
      os << "-";
    }
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
