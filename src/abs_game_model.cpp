#include "abs_game_model.h"

#include <sstream>

static void expect_good(const std::istream &is) {
  if (!is.good()) {
    throw ParseFailure("stream error");
  }
}

AbsCards::AbsCards(std::string_view s) {
  std::istringstream is(std::string(s), std::ios_base::in);
  is >> *this;
}

std::ostream &operator<<(std::ostream &os, const AbsCards &c) {
  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    os << suit << ' ';
    Cards hc = c.high_cards().intersect(suit);
    int   lc = c.low_cards(suit);
    if (hc.empty() && lc == 0) {
      os << '-';
    } else {
      for (auto it = hc.iter_highest(); it.valid(); it = hc.iter_lower(it)) {
        os << it.card().rank();
      }
      for (int i = 0; i < lc; i++) {
        os << 'X';
      }
    }
    if (suit > FIRST_SUIT) {
      os << ' ';
    }
  }
  return os;
}

static void expect_suit(std::istream &is, Suit suit) {
  Suit s;
  expect_good(is >> s);
  if (s != suit) {
    throw ParseFailure("wrong suit");
  }
}

std::istream &operator>>(std::istream &is, AbsCards &c) {
  Cards  hc;
  int8_t lc[4] = {0};

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    expect_suit(is, suit);
    expect_good(is >> std::ws);
    if (is.peek() == '-') {
      char dummy;
      expect_good(is >> dummy);
      continue;
    } else {
      while (peek_is_rank(is)) {
        Rank rank;
        expect_good(is >> rank);
        if (rank == RANK_UNKNOWN) {
          lc[suit]++;
        } else {
          hc.add(Card(rank, suit));
        }
      }
    }
  }

  c = AbsCards(hc, lc);

  return is;
}
