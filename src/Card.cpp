#include "Card.h"

#include <iostream>

#include "Panic.h"

Card::Rank ParseRank(char ch) {
  if (ch >= '2' && ch <= '9') {
    return static_cast<Card::Rank>(ch - '2');
  }
  switch (ch) {
    case 'T':
      return Card::Rank::T;
    case 'J':
      return Card::Rank::J;
    case 'Q':
      return Card::Rank::Q;
    case 'K':
      return Card::Rank::K;
    case 'A':
      return Card::Rank::A;
    default:
      panic("bad rank");
      exit(1);
  }
}

Card::Suit ParseSuit(std::string_view utf8_str) {
  if (utf8_str == "C" || utf8_str == "♣") {
    return Card::Suit::C;
  } else if (utf8_str == "D" || utf8_str == "♦") {
    return Card::Suit::D;
  } else if (utf8_str == "H" || utf8_str == "♥") {
    return Card::Suit::H;
  } else if (utf8_str == "S" || utf8_str == "♠") {
    return Card::Suit::S;
  } else {
    panic("bad suit");
    exit(1);
  }
}

Card::Card(std::string_view utf8_str) {
  if (utf8_str.length() < 2) {
    panic("bad card");
  }
  Rank r = ParseRank(utf8_str[0]);
  Suit s = ParseSuit(utf8_str.substr(1));
  _card = ((uint64_t)1) << (r + s * 13);
}

std::ostream& operator<<(std::ostream& os, const Card::Suit& s) {
  switch (s) {
    case Card::C:
      os << "♣";
      break;
    case Card::D:
      os << "♦";
      break;
    case Card::H:
      os << "♥";
      break;
    case Card::S:
      os << "♠";
      break;
    default:
      panic("bad suit");
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Card::Rank& r) {
  if (r >= 0 && r < 8) {
    os << static_cast<int>(r + 2);
  } else {
    switch (r) {
      case Card::T:
        os << "T";
        break;
      case Card::J:
        os << "J";
        break;
      case Card::Q:
        os << "Q";
        break;
      case Card::K:
        os << "K";
        break;
      case Card::A:
        os << "A";
        break;
      default:
        panic("bad rank");
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Card& c) {
  os << c.GetRank() << c.GetSuit();
  return os;
}
