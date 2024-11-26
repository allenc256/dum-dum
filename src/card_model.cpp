#include "card_model.h"
#include <algorithm>
#include <iostream>
#include <sstream>

static Suit parse_suit(std::istream &is) {
  char ch;
  is >> ch;
  if (!is.good()) {
    throw ParseFailure("stream error");
  }
  switch ((uint8_t)ch) {
  case 'C': return Suit::CLUBS;
  case 'D': return Suit::DIAMONDS;
  case 'H': return Suit::HEARTS;
  case 'S': return Suit::SPADES;
  case 'N':
    if (!is.get(ch) || ch != 'T') {
      throw ParseFailure("bad suit");
    }
    return Suit::NO_TRUMP;
  case 0xE2:
    if (!is.get(ch) || (uint8_t)ch != 0x99) {
      throw ParseFailure("bad suit (utf8 sequence)");
    }
    is.get(ch);
    switch ((uint8_t)ch) {
    case 0xA3: return Suit::CLUBS;
    case 0xA6: return Suit::DIAMONDS;
    case 0xA5: return Suit::HEARTS;
    case 0xA0: return Suit::SPADES;
    }
  }
  throw ParseFailure("bad suit");
}

std::istream &operator>>(std::istream &is, Suit &s) {
  s = parse_suit(is);
  return is;
}

static constexpr std::string_view SUIT_STRS[]     = {"♣", "♦", "♥", "♠", "NT"};
static constexpr std::string_view SUIT_STRS_ASC[] = {"C", "D", "H", "S", "NT"};

std::string_view to_ascii(Suit suit) { return SUIT_STRS_ASC[suit]; }

std::ostream &operator<<(std::ostream &os, Suit s) {
  if (s < 0 || s >= 5) {
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
  case 'T': return Rank::TEN;
  case 'J': return Rank::JACK;
  case 'Q': return Rank::QUEEN;
  case 'K': return Rank::KING;
  case 'A': return Rank::ACE;
  default: throw ParseFailure("bad rank");
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
  if (s == NO_TRUMP) {
    throw ParseFailure("bad suit");
  }
  c = Card(r, s);
  return is;
}

std::ostream &operator<<(std::ostream &os, Card c) {
  os << c.rank() << c.suit();
  return os;
}

Card::Card(std::string_view s) {
  std::istringstream is(std::string(s), std::ios_base::in);
  is >> *this;
}

Card::Card(const char *s) : Card(std::string_view(s)) {}

std::string Card::to_string() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}

std::ostream &operator<<(std::ostream &os, Cards cards) {
  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    if (suit != LAST_SUIT) {
      os << '.';
    }
    for (Card card : cards.intersect(suit).high_to_low()) {
      os << card.rank();
    }
  }
  return os;
}

bool is_rank_char(int ch) {
  if (ch >= '2' && ch <= '9') {
    return true;
  } else {
    switch (ch) {
    case 'T': return true;
    case 'J': return true;
    case 'Q': return true;
    case 'K': return true;
    case 'A': return true;
    }
  }
  return false;
}

static void parse_cards_ranks(std::istream &is, Suit suit, Cards &cards) {
  while (true) {
    is >> std::ws;
    int ch = is.peek();
    if (is_rank_char(ch)) {
      Rank rank;
      is >> rank;
      cards.add(Card(rank, suit));
    } else {
      break;
    }
  }
}

std::istream &operator>>(std::istream &is, Cards &cards) {
  cards = Cards();
  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    if (suit != LAST_SUIT) {
      char delim = 0;
      is >> delim;
      if (delim != '.') {
        throw ParseFailure("expected delimiter: .");
      }
    }
    parse_cards_ranks(is, suit, cards);
  }
  return is;
}

Cards::Cards(std::string_view s) {
  std::istringstream is(std::string(s), std::ios_base::in);
  is >> *this;
}

std::string Cards::to_string() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}
