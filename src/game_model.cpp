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
  if ((int)s < 0 || (int)s >= 4) {
    throw std::runtime_error("bad seat");
  }
  os << SEAT_STR[s];
  return os;
}

std::istream &operator>>(std::istream &is, Seat &s) {
  char ch;
  is >> std::ws >> ch;
  switch (ch) {
  case 'W': s = WEST; break;
  case 'N': s = NORTH; break;
  case 'E': s = EAST; break;
  case 'S': s = SOUTH; break;
  default: throw ParseFailure("bad seat");
  }
  return is;
}

std::ostream &operator<<(std::ostream &os, const Trick &t) {
  if (!t.started()) {
    os << '-';
  } else {
    for (int i = 0; i < t.card_count(); i++) {
      if (t.is_null_play(i)) {
        os << "??";
      } else {
        os << t.card(i);
      }
    }
    if (t.finished()) {
      os << " " << t.winning_seat();
    }
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const Game &g) {
  constexpr int spacing = 15;

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, g.hands_[NORTH], suit);
    os << std::endl;
  }

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    int count = print_cards_in_suit(os, g.hands_[WEST], suit);
    print_chars(os, 2 * spacing - count, ' ');
    print_cards_in_suit(os, g.hands_[EAST], suit);
    os << std::endl;
  }

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, g.hands_[SOUTH], suit);
    os << std::endl;
  }

  print_chars(os, spacing * 3, '-');
  os << std::endl;
  os << "trump_suit:         " << g.trump_suit_ << std::endl;
  os << "lead_seat:          " << g.lead_seat_ << std::endl;
  os << "trick:              ";
  if (g.current_trick().started()) {
    os << g.current_trick() << std::endl;
  } else if (g.tricks_taken_ > 0) {
    os << g.tricks_[g.tricks_taken_ - 1] << std::endl;
  } else {
    os << "-" << std::endl;
  }
  os << "next_seat:          " << g.next_seat_ << std::endl;
  os << "tricks_taken_by_ns: " << g.tricks_taken_by_ns_ << std::endl;
  os << "tricks_taken_by_ew: " << g.tricks_taken_by_ew() << std::endl;
  os << "tricks_max:         " << g.tricks_max_ << std::endl;
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
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      c[seat].add(indexes[i + 13 * seat]);
    }
  }

  std::uniform_int_distribution<> sd(0, 4);
  std::uniform_int_distribution<> dd(0, 3);
  Suit                            trump_suit = (Suit)sd(random);
  Seat                            declarer   = (Seat)dd(random);

  return Game(trump_suit, declarer, c);
}

Game::Game(Suit trump_suit, Seat lead_seat, Cards hands[4])
    : trump_suit_(trump_suit),
      lead_seat_(lead_seat),
      next_seat_(lead_seat),
      tricks_taken_(0),
      tricks_taken_by_ns_(0),
      game_state_valid_(false) {
  tricks_max_ = hands[0].count();
  for (int i = 1; i < 4; i++) {
    if (hands[i].count() != tricks_max_) {
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
  if (tricks_taken_ >= tricks_max_) {
    return false;
  }
  Cards        cs = hands_[next_seat_];
  const Trick &t  = current_trick();
  if (!cs.contains(c)) {
    return false;
  }
  if (!t.started()) {
    return true;
  }
  return c.suit() == t.lead_suit() || cs.intersect(t.lead_suit()).empty();
}

void Game::play(Card c) {
  assert(valid_play(c));

  Trick &t = current_trick();

  if (t.started()) {
    t.play_continue(c);
  } else {
    t.play_start(trump_suit_, next_seat_, c);
  }

  hands_[next_seat_].remove(c);

  if (t.finished()) {
    next_seat_ = t.winning_seat();
    if (next_seat_ == NORTH || next_seat_ == SOUTH) {
      tricks_taken_by_ns_++;
    }
    tricks_taken_++;
  } else {
    next_seat_ = t.next_seat();
  }

  game_state_valid_ = false;
}

void Game::play_null() {
  Trick &t = current_trick();
  assert(t.started());
  t.play_null();

  if (t.finished()) {
    next_seat_ = t.winning_seat();
    if (next_seat_ == NORTH || next_seat_ == SOUTH) {
      tricks_taken_by_ns_++;
    }
    tricks_taken_++;
  } else {
    next_seat_ = t.next_seat();
  }

  game_state_valid_ = false;
}

void Game::unplay() {
  Trick &t = current_trick();
  if (t.started()) {
    assert(!t.finished());
    Trick::Unplay u = t.unplay();
    if (t.started()) {
      next_seat_ = t.next_seat();
    } else if (tricks_taken_ > 0) {
      next_seat_ = tricks_[tricks_taken_ - 1].winning_seat();
    } else {
      next_seat_ = lead_seat_;
    }
    if (!u.was_null_play) {
      hands_[next_seat_].add(u.card);
    }
  } else {
    if (tricks_taken_ > 0) {
      tricks_taken_--;
      Trick &t = current_trick();
      assert(t.finished());
      Seat winner = t.winning_seat();
      if (winner == NORTH || winner == SOUTH) {
        tricks_taken_by_ns_--;
      }
      Trick::Unplay u = t.unplay();
      next_seat_      = t.next_seat();
      if (!u.was_null_play) {
        hands_[next_seat_].add(u.card);
      }
    } else {
      throw std::runtime_error("no cards played");
    }
  }
  game_state_valid_ = false;
}

Cards Game::valid_plays() const {
  if (finished()) {
    return Cards();
  }

  Cards        c = hands_[next_seat_];
  const Trick &t = current_trick();
  if (t.started()) {
    Cards cs = c.intersect(t.lead_suit());
    if (!cs.empty()) {
      return cs;
    }
  }
  return c;
}