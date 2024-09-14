#include "Game.h"

#include <random>

#include "Panic.h"

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
        os << ", ";
      }
      os << t.card(i) << " by " << t.seat(i);
    }
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const Game &g) {
  constexpr int spacing = 15;

  for (int i = 3; i >= 0; i--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, g.cards_[NORTH], (Suit)i);
    os << std::endl;
  }

  for (int i = 3; i >= 0; i--) {
    int count = print_cards_in_suit(os, g.cards_[WEST], (Suit)i);
    print_chars(os, 2 * spacing - count, ' ');
    print_cards_in_suit(os, g.cards_[EAST], (Suit)i);
    os << std::endl;
  }

  for (int i = 3; i >= 0; i--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, g.cards_[SOUTH], (Suit)i);
    os << std::endl;
  }

  print_chars(os, spacing * 3, '-');
  os << std::endl;
  os << "contract:           " << g.contract_ << std::endl;
  os << "trick:              " << g.current_trick() << std::endl;
  os << "next_player:        " << g.next_player_ << std::endl;
  os << "tricks_taken_by_ns: " << g.tricks_taken_by_ns_ << std::endl;
  os << "tricks_taken_by_ew: " << (g.trick_count_ - g.tricks_taken_by_ns_)
     << std::endl;
  print_chars(os, spacing * 3, '-');
  os << std::endl;

  return os;
}

Game Game::random_deal(std::default_random_engine &random) {
  int indexes[52];
  for (int i = 0; i < 52; i++) {
    indexes[i] = i;
  }
  std::shuffle(indexes, indexes + 52, random);

  Cards c[4];
  for (int i = 0; i < 13; i++) {
    for (int j = 0; j < 4; j++) {
      c[j].add(indexes[i + 13 * j]);
    }
  }

  return Game(Contract(3, NO_TRUMP, NORTH), c);
}

Game::Game(Contract contract, Cards cards[4])
    : contract_(contract), next_player_(left_seat(contract.declarer())),
      trick_count_(0), tricks_taken_by_ns_(0) {
  trick_max_count_ = cards[0].count();
  for (int i = 1; i < 4; i++) {
    if (cards[i].count() != trick_max_count_) {
      throw new std::runtime_error("mismatched hand sizes");
    }
  }

  for (int i = 0; i < 4; i++) {
    cards_[i] = cards[i];
  }
}

bool Game::valid_play(Card c) const {
  if (trick_count_ >= trick_max_count_) {
    return false;
  }
  Cards cs = cards_[next_player_];
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
    t.play_start(next_player_, c);
  }

  cards_[next_player_].remove(c);

  if (t.finished()) {
    trick_count_++;
    next_player_ = winner(t);
  } else {
    next_player_ = right_seat(next_player_);
  }
}

inline bool better_card(Card c, Card winner, Suit lead_suit, Suit trump_suit) {
  if (trump_suit == NO_TRUMP) {
    return c.suit() == lead_suit && c.rank() > winner.rank();
  } else {
    if (c.suit() == lead_suit) {
      return c.rank() > winner.rank();
    } else if (c.suit() == trump_suit) {
      if (winner.suit() == trump_suit) {
        return c.rank() > winner.rank();
      } else {
        return true;
      }
    } else {
      return false;
    }
  }
}

Seat Game::winner(const Trick &t) const {
  assert(t.finished());

  Card winner_card = t.card(0);
  Seat winner_seat = t.lead_seat();
  for (int i = 1; i < 4; i++) {
    Card c = t.card(i);
    if (better_card(c, winner_card, t.lead_suit(), contract_.suit())) {
      winner_card = c;
      winner_seat = right_seat(t.lead_seat(), i);
    }
  }

  return winner_seat;
}

void Game::unplay() {
  // empty
}
