#include "Cards.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Panic.h"

static Suit parse_suit(std::istream &is) {
  char ch;
  is >> ch;
  if (!is.good()) {
    throw ParseFailure("stream error");
  }
  switch ((uint8_t)ch) {
  case 'C':
    return Suit::CLUBS;
  case 'D':
    return Suit::DIAMONDS;
  case 'H':
    return Suit::HEARTS;
  case 'S':
    return Suit::SPADES;
  case 0xE2:
    if (!is.get(ch) || (uint8_t)ch != 0x99) {
      throw ParseFailure("bad suit (utf8 sequence)");
    }
    is.get(ch);
    switch ((uint8_t)ch) {
    case 0xA3:
      return Suit::CLUBS;
    case 0xA6:
      return Suit::DIAMONDS;
    case 0xA5:
      return Suit::HEARTS;
    case 0xA0:
      return Suit::SPADES;
    }
  }
  throw ParseFailure("bad suit");
}

std::istream &operator>>(std::istream &is, Suit &s) {
  s = parse_suit(is);
  return is;
}

static const char *SUIT_STRS[] = {"♣", "♦", "♥", "♠"};

std::ostream &operator<<(std::ostream &os, Suit s) {
  if (s < 0 || s >= 4) {
    throw std::runtime_error("bad suit");
  }
  os << SUIT_STRS[s];
  return os;
}

static Rank parse_rank(std::istream &is) {
  char ch;
  is >> ch;
  if (!is.good()) {
    throw ParseFailure("stream error");
  }
  if (ch >= '2' && ch <= '9') {
    return (Rank)(ch - '2');
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
    throw ParseFailure("bad rank");
  }
}

std::istream &operator>>(std::istream &is, Rank &r) {
  r = parse_rank(is);
  return is;
}

static const char *RANK_STRS = "23456789TJQKA";

std::ostream &operator<<(std::ostream &os, Rank r) {
  if (r < 0 || r >= 13) {
    throw std::runtime_error("bad rank");
  }
  os << RANK_STRS[r];
  return os;
}

std::istream &operator>>(std::istream &is, Card &c) {
  Rank r = parse_rank(is);
  Suit s = parse_suit(is);
  c.rank_ = r;
  c.suit_ = s;
  return is;
}

std::ostream &operator<<(std::ostream &os, Card c) {
  os << c.rank() << c.suit();
  return os;
}

Card::Card(std::string s) {
  std::istringstream is(s);
  is >> *this;
}

std::string Card::to_string() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}

int print_cards_in_suit(std::ostream &os, Cards c, Suit s) {
  c = c.intersect_suit(s);
  os << s << " ";
  int count = 0;
  for (auto i = c.first(); i.valid(); i = c.next(i)) {
    os << i.card().rank();
    count++;
  }
  if (count <= 0) {
    os << "-";
    count++;
  }
  return count + 2;
}

std::ostream &operator<<(std::ostream &os, Cards c) {
  for (int s = 3; s >= 0; s--) {
    if (s < 3) {
      os << " ";
    }
    print_cards_in_suit(os, c, (Suit)s);
  }
  return os;
}

bool is_rank_char(int ch) {
  if (ch >= '2' && ch <= '9') {
    return true;
  } else {
    switch (ch) {
    case 'T':
      return true;
    case 'J':
      return true;
    case 'Q':
      return true;
    case 'K':
      return true;
    case 'A':
      return true;
    }
  }
  return false;
}

static void parse_cards_ranks(std::istream &is, Suit s, Cards &cs) {
  Rank r;
  Rank last_rank;
  int ch;
  int count = 0;

  is >> std::ws;
  while (true) {
    ch = is.peek();
    if (ch == EOF) {
      if (count == 0) {
        throw ParseFailure("EOF");
      }
      return;
    }
    if (!is.good()) {
      throw ParseFailure("stream error");
    }
    if (is_rank_char(ch)) {
      is >> r;
      if (count > 0 && r >= last_rank) {
        throw ParseFailure("order");
      }
      cs.add(Card(r, s));
      last_rank = r;
      count++;
    } else if (ch == '-') {
      is.get();
      return;
    } else {
      return;
    }
  }
}

std::istream &operator>>(std::istream &is, Cards &c) {
  c.clear();
  for (int i = 3; i >= 0; i--) {
    Suit s;
    is >> s;
    if (s != (Suit)i) {
      throw ParseFailure("bad suit");
    }
    parse_cards_ranks(is, s, c);
  }
  return is;
}
