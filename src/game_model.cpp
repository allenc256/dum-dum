#include "game_model.h"

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
      if (t.has_card(i)) {
        os << t.card(i);
      } else {
        os << "??";
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
    print_cards_in_suit(os, g.hand(NORTH), suit);
    os << std::endl;
  }

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    int count = print_cards_in_suit(os, g.hand(WEST), suit);
    print_chars(os, 2 * spacing - count, ' ');
    print_cards_in_suit(os, g.hand(EAST), suit);
    os << std::endl;
  }

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, g.hand(SOUTH), suit);
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

Game::Game(Suit trump_suit, Seat lead_seat, const Hands &hands)
    : hands_(hands),
      trump_suit_(trump_suit),
      lead_seat_(lead_seat),
      next_seat_(lead_seat),
      tricks_taken_(0),
      tricks_max_(hands.hand(WEST).count()),
      tricks_taken_by_ns_(0) {
  if (!hands.all_same_size()) {
    throw std::runtime_error("hands must be same size");
  }
  if (!hands.all_disjoint()) {
    throw std::runtime_error("hands must be disjoint");
  }

  Cards missing_cards = hands.all_cards().complement();
  card_normalizer_.remove_all(missing_cards);
}

bool Game::valid_play(Card c) const {
  if (tricks_taken_ >= tricks_max_) {
    return false;
  }
  Cards        cs = hand(next_seat_);
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
  hands_.remove_card(next_seat_, c);
  finish_play();
}

void Game::play_null() {
  Trick &t = current_trick();
  assert(t.started());
  t.play_null();
  finish_play();
}

void Game::finish_play() {
  Trick &t = current_trick();
  if (t.finished()) {
    next_seat_ = t.winning_seat();
    if (next_seat_ == NORTH || next_seat_ == SOUTH) {
      tricks_taken_by_ns_++;
    }
    for (int i = 0; i < 4; i++) {
      if (t.has_card(i)) {
        card_normalizer_.remove(t.card(i));
      }
    }
    tricks_taken_++;
    assert(tricks_taken_ <= 13);
    assert(!norm_key_stack_[tricks_taken_].has_value());
    assert(!norm_hands_stack_[tricks_taken_].has_value());
  } else {
    next_seat_ = t.next_seat();
  }
}

void Game::unplay() {
  Trick &t = current_trick();
  if (t.started()) {
    assert(!t.finished());
    std::optional<Card> c = t.unplay();
    if (t.started()) {
      next_seat_ = t.next_seat();
    } else if (tricks_taken_ > 0) {
      next_seat_ = last_finished_trick().winning_seat();
    } else {
      next_seat_ = lead_seat_;
    }
    if (c.has_value()) {
      hands_.add_card(next_seat_, *c);
    }
  } else {
    if (tricks_taken_ > 0) {
      Trick &t = last_finished_trick();
      assert(t.finished());
      for (int i = 0; i < 4; i++) {
        if (t.has_card(i)) {
          card_normalizer_.add(t.card(i));
        }
      }
      Seat                winner = t.winning_seat();
      std::optional<Card> card   = t.unplay();
      next_seat_                 = t.next_seat();
      if (card.has_value()) {
        hands_.add_card(next_seat_, *card);
      }
      if (winner == NORTH || winner == SOUTH) {
        tricks_taken_by_ns_--;
      }
      norm_key_stack_[tricks_taken_].reset();
      norm_hands_stack_[tricks_taken_].reset();
      tricks_taken_--;
    } else {
      throw std::runtime_error("no cards played");
    }
  }
}

Cards Game::valid_plays() const {
  if (finished()) {
    return Cards();
  }

  Cards        c = hand(next_seat_);
  const Trick &t = current_trick();
  if (t.started()) {
    Cards cs = c.intersect(t.lead_suit());
    if (!cs.empty()) {
      return cs;
    }
  }
  return c;
}

const GameKey &Game::normalized_key() {
  assert(start_of_trick());
  auto &cached = norm_key_stack_[tricks_taken_];
  if (!cached.has_value()) {
    cached.emplace(normalized_hands(), next_seat_);
  }
  return *cached;
}

const Hands &Game::normalized_hands() {
  assert(start_of_trick());
  auto &cached = norm_hands_stack_[tricks_taken_];
  if (!cached.has_value()) {
    cached.emplace(
        card_normalizer_.normalize(hands_.hand(WEST)),
        card_normalizer_.normalize(hands_.hand(NORTH)),
        card_normalizer_.normalize(hands_.hand(EAST)),
        card_normalizer_.normalize(hands_.hand(SOUTH))
    );
  }
  return *cached;
}
