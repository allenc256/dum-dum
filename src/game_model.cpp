#include "game_model.h"
#include <random>

static void print_chars(std::ostream &os, int n, char ch) {
  for (int i = 0; i < n; i++) {
    os << ch;
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

std::istream &operator>>(std::istream &is, Seat &s) {
  char ch;
  is >> ch;
  switch (ch) {
  case 'W':
    s = WEST;
    break;
  case 'N':
    s = NORTH;
    break;
  case 'E':
    s = EAST;
    break;
  case 'S':
    s = SOUTH;
    break;
  }
  throw ParseFailure("bad seat");
}

std::ostream &operator<<(std::ostream &os, Contract c) {
  os << (int)c.level() << c.suit() << " by " << c.declarer();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Trick &t) {
  if (t.card_count() == 0) {
    os << "-";
  } else {
    for (int i = 0; i < t.card_count(); i++) {
      if (i > 0) {
        os << " ";
      }
      os << t.seat(i) << ":" << t.card(i);
    }
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const Game &g) {
  constexpr int spacing = 15;

  for (int i = 3; i >= 0; i--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, g.hands_[NORTH], (Suit)i);
    os << std::endl;
  }

  for (int i = 3; i >= 0; i--) {
    int count = print_cards_in_suit(os, g.hands_[WEST], (Suit)i);
    print_chars(os, 2 * spacing - count, ' ');
    print_cards_in_suit(os, g.hands_[EAST], (Suit)i);
    os << std::endl;
  }

  for (int i = 3; i >= 0; i--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, g.hands_[SOUTH], (Suit)i);
    os << std::endl;
  }

  print_chars(os, spacing * 3, '-');
  os << std::endl;
  os << "contract:           " << g.contract_ << std::endl;
  os << "trick:              ";
  if (g.current_trick().started()) {
    os << g.current_trick() << std::endl;
  } else if (g.trick_count_ > 0) {
    os << g.tricks_[g.trick_count_ - 1] << std::endl;
  } else {
    os << "-" << std::endl;
  }
  os << "next_seat:          " << g.next_seat_ << std::endl;
  os << "tricks_taken_by_ns: " << g.tricks_taken_by_ns_ << std::endl;
  os << "tricks_taken_by_ew: " << (g.trick_count_ - g.tricks_taken_by_ns_)
     << std::endl;
  os << "tricks_max_count:   " << g.trick_max_count_ << std::endl;
  print_chars(os, spacing * 3, '-');
  os << std::endl;

  return os;
}

Game Game::random_deal(std::default_random_engine &random, int cards_per_hand) {
  int indexes[52];
  for (int i = 0; i < 52; i++) {
    indexes[i] = i;
  }
  std::shuffle(indexes, indexes + 52, random);

  Cards c[4];
  for (int i = 0; i < cards_per_hand; i++) {
    for (int j = 0; j < 4; j++) {
      c[j].add(indexes[i + 13 * j]);
    }
  }

  std::uniform_int_distribution<> cd(1, 7);
  std::uniform_int_distribution<> sd(0, 4);
  std::uniform_int_distribution<> dd(0, 3);
  int level = cd(random);
  Suit suit = (Suit)sd(random);
  Seat declarer = (Seat)dd(random);

  return Game(Contract(level, suit, declarer), c);
}

Game::Game(Contract contract, Cards hands[4])
    : contract_(contract), next_seat_(left_seat(contract.declarer())),
      trick_count_(0), tricks_taken_by_ns_(0) {
  trick_max_count_ = hands[0].count();
  for (int i = 1; i < 4; i++) {
    if (hands[i].count() != trick_max_count_) {
      throw std::runtime_error("hands must be same size");
    }
  }

  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      if (!hands[i].disjoint(hands[j])) {
        throw std::runtime_error("hands must be disjoint");
      }
    }
  }

  for (int i = 0; i < 4; i++) {
    hands_[i] = hands[i];
  }
}

bool Game::valid_play(Card c) const {
  if (trick_count_ >= trick_max_count_) {
    return false;
  }
  Cards cs = hands_[next_seat_];
  const Trick &t = current_trick();
  if (!cs.contains(c)) {
    return false;
  }
  if (!t.started()) {
    return true;
  }
  return c.suit() == t.lead_suit() || cs.intersect_suit(t.lead_suit()).empty();
}

void Game::play(Card c) {
  assert(valid_play(c));

  Trick &t = current_trick();

  if (t.started()) {
    t.play_continue(c);
  } else {
    t.play_start(contract_.suit(), next_seat_, c);
  }

  hands_[next_seat_].remove(c);

  next_seat_ = t.next_seat();

  if (t.finished()) {
    if (next_seat_ == NORTH || next_seat_ == SOUTH) {
      tricks_taken_by_ns_++;
    }
    trick_count_++;
  }
}

void Game::unplay() {
  Trick &t = current_trick();
  if (t.started()) {
    assert(!t.finished());
    Card c = t.unplay();
    next_seat_ = t.next_seat();
    hands_[next_seat_].add(c);
  } else {
    if (trick_count_ > 0) {
      trick_count_--;
      Trick &t = current_trick();
      assert(t.finished());
      Seat winner = t.next_seat();
      if (winner == NORTH || winner == SOUTH) {
        tricks_taken_by_ns_--;
      }
      Card c = t.unplay();
      next_seat_ = t.next_seat();
      hands_[next_seat_].add(c);
    } else {
      throw std::runtime_error("no cards played");
    }
  }
}

Cards Game::valid_plays() const {
  if (finished()) {
    return Cards();
  }

  Cards c = hands_[next_seat_];
  const Trick &t = current_trick();
  if (t.started()) {
    Cards cs = c.intersect_suit(t.lead_suit());
    if (!cs.empty()) {
      return cs;
    }
  }
  return c;
}