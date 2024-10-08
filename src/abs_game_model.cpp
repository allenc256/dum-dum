#include "abs_game_model.h"

#include <iomanip>
#include <sstream>

static void expect_good(const std::istream &is) {
  if (!is.good()) {
    throw ParseFailure("stream error");
  }
}

std::ostream &operator<<(std::ostream &os, const AbsTrick &t) {
  if (t.state_ == AbsTrick::STARTING) {
    os << '-';
    return os;
  }
  for (int i = 0; i < t.card_count_; i++) {
    os << t.cards_[i];
  }
  if (t.state_ == AbsTrick::FINISHED) {
    os << ' ' << t.winning_seat_;
  }
  return os;
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

static void print_spaces(std::ostream &os, int n) {
  for (int i = 0; i < n; i++) {
    os << ' ';
  }
}

static int
print_cards_in_suit(std::ostream &os, const AbsCards &cards, Suit suit) {
  os << suit << ' ';
  Cards hc = cards.high_cards().intersect(suit);
  int   lc = cards.low_cards(suit);
  if (hc.empty() && lc == 0) {
    os << '-';
    return 3;
  }
  for (auto it = hc.iter_highest(); it.valid(); it = hc.iter_lower(it)) {
    os << it.card().rank();
  }
  for (int i = 0; i < lc; i++) {
    os << 'X';
  }
  return hc.count() + lc + 2;
}

std::ostream &operator<<(std::ostream &os, const AbsGame &g) {
  constexpr int spacing = 15;

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    print_spaces(os, spacing);
    print_cards_in_suit(os, g.hands_[NORTH], suit);
    os << std::endl;
  }

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    int n = print_cards_in_suit(os, g.hands_[WEST], suit);
    print_spaces(os, spacing * 2 - n);
    print_cards_in_suit(os, g.hands_[EAST], suit);
    os << std::endl;
  }

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    print_spaces(os, spacing);
    print_cards_in_suit(os, g.hands_[SOUTH], suit);
    os << std::endl;
  }

  int trick_index = g.tricks_taken_;
  if (g.tricks_[trick_index].state() == AbsTrick::STARTING && trick_index > 0) {
    trick_index--;
  }

  os << std::endl;
  os << std::left;
  os << std::setw(20) << "trump_suit:" << g.trump_suit_ << std::endl;
  os << std::setw(20) << "next_seat:" << g.next_seat_ << std::endl;
  os << std::setw(20) << "current_trick:" << g.tricks_[trick_index]
     << std::endl;
  os << std::setw(20) << "tricks_taken:" << g.tricks_taken_ << std::endl;
  os << std::setw(20) << "tricks_taken_by_ns:" << g.tricks_taken_by_ns_
     << std::endl;
  os << std::setw(20) << "tricks_max:" << g.tricks_max_ << std::endl;

  return os;
}