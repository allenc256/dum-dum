#include "Card.h"

#include <iostream>

#include "Panic.h"

Rank ParseRank(char ch) {
  if (ch >= '2' && ch <= '9') {
    return static_cast<Rank>(ch - '2');
  }
  switch (ch) {
    case 'T':
      return Rank::T;
    case 'J':
      return Rank::J;
    case 'Q':
      return Rank::Q;
    case 'K':
      return Rank::K;
    case 'A':
      return Rank::A;
    default:
      PANIC("bad rank");
  }
}

Suit ParseSuit(std::string_view utf8_str) {
  if (utf8_str == "C" || utf8_str == "♣") {
    return Suit::C;
  } else if (utf8_str == "D" || utf8_str == "♦") {
    return Suit::D;
  } else if (utf8_str == "H" || utf8_str == "♥") {
    return Suit::H;
  } else if (utf8_str == "S" || utf8_str == "♠") {
    return Suit::S;
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

std::ostream& operator<<(std::ostream& os, const Suit& s) {
  switch (s) {
    case Suit::C:
      os << "♣";
      break;
    case Suit::D:
      os << "♦";
      break;
    case Suit::H:
      os << "♥";
      break;
    case Suit::S:
      os << "♠";
      break;
    default:
      PANIC("bad suit");
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Rank& r) {
  if (r >= 0 && r < 8) {
    os << static_cast<int>(r + 2);
  } else {
    switch (r) {
      case Rank::T:
        os << "T";
        break;
      case Rank::J:
        os << "J";
        break;
      case Rank::Q:
        os << "Q";
        break;
      case Rank::K:
        os << "K";
        break;
      case Rank::A:
        os << "A";
        break;
      default:
        PANIC("bad rank");
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Card& c) {
  os << c.rank() << c.suit();
  return os;
}

std::ostream& operator<<(std::ostream& os, const Cards& c) {
  for (int s = 0; s < 4; s++) {
    Cards cs = c.of_suit((Suit)s);

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