#include "game_model.h"

#include <sstream>

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

Hands::Hands(std::string_view s) {
  std::istringstream is(std::string(s), std::ios_base::in);
  is >> *this;
}

std::ostream &operator<<(std::ostream &os, const Hands &hands) {
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    if (seat != FIRST_SEAT) {
      os << '/';
    }
    Cards hand = hands.hand(seat);
    for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
      if (suit != LAST_SUIT) {
        os << '.';
      }
      for (Card card : hand.intersect(suit).high_to_low()) {
        os << card.rank();
      }
    }
  }
  return os;
}

std::istream &operator>>(std::istream &is, Hands &hands) {
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    if (seat != FIRST_SEAT && is.get() != '/') {
      throw ParseFailure("expected '/' delimiter");
    }
    for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
      if (suit != LAST_SUIT && is.get() != '.') {
        throw ParseFailure("expected '.' delimiter");
      }
      while (is.peek() != EOF && is.peek() != '.' && is.peek() != '/') {
        Rank rank;
        is >> rank;
        hands.hands_[seat].add(Card(rank, suit));
      }
    }
  }
  return is;
}

static void print_chars(std::ostream &os, int n, char ch) {
  for (int i = 0; i < n; i++) {
    os << ch;
  }
}

static int print_cards_in_suit(
    std::ostream &os, Cards cards, Suit suit, Cards winners_by_rank
) {
  cards = cards.intersect(suit);
  os << suit << ' ';
  if (cards.empty()) {
    os << '-';
    return 3;
  }
  for (Card c : cards.high_to_low()) {
    if (winners_by_rank.contains(c)) {
      os << c.rank();
    } else {
      os << 'X';
    }
  }
  return 2 + cards.count();
}

void Hands::pretty_print(std::ostream &os, Cards winners_by_rank) const {
  constexpr int spacing = 15;

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, hand(NORTH), suit, winners_by_rank);
    os << '\n';
  }

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    int count = print_cards_in_suit(os, hand(WEST), suit, winners_by_rank);
    print_chars(os, 2 * spacing - count, ' ');
    print_cards_in_suit(os, hand(EAST), suit, winners_by_rank);
    os << '\n';
  }

  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    print_chars(os, spacing, ' ');
    print_cards_in_suit(os, hand(SOUTH), suit, winners_by_rank);
    os << '\n';
  }
}

std::string Hands::to_string() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}

void Game::pretty_print(std::ostream &os) const {
  hands_.pretty_print(os, Cards::all());
  os << '\n';
  os << "trump_suit:         " << trump_suit_ << '\n';
  os << "next_seat:          " << next_seat_ << '\n';
  os << "tricks_taken_by_ns: " << tricks_taken_by_ns_ << '\n';
  os << "tricks_taken_by_ew: " << tricks_taken_by_ew() << '\n';
}

std::ostream &operator<<(std::ostream &os, const Game &g) {
  g.pretty_print(os);
  return os;
}

Game::Game(Suit trump_suit, Seat lead_seat, const Hands &hands)
    : hands_(hands),
      trump_suit_(trump_suit),
      lead_seat_(lead_seat),
      next_seat_(lead_seat),
      tricks_taken_(0),
      tricks_max_(hands.hand(FIRST_SEAT).count()),
      tricks_taken_by_ns_(0) {
  if (!hands.all_same_size()) {
    throw std::runtime_error("hands must be same size");
  }
  if (!hands.all_disjoint()) {
    throw std::runtime_error("hands must be disjoint");
  }

  card_normalizer_.remove_all(hands.all_cards().complement());
}

bool Game::valid_play(Card c) const {
  if (tricks_taken_ >= tricks_max_) {
    return false;
  }
  Cards        cs = hands_.hand(next_seat_);
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
    assert(!state_stack_[tricks_taken_].has_value());
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
      next_seat_ = tricks_[tricks_taken_ - 1].winning_seat();
    } else {
      next_seat_ = lead_seat_;
    }
    if (c.has_value()) {
      hands_.add_card(next_seat_, *c);
    }
  } else {
    if (tricks_taken_ > 0) {
      Trick &t = tricks_[tricks_taken_ - 1];
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
      state_stack_[tricks_taken_].reset();
      norm_hands_stack_[tricks_taken_].reset();
      tricks_taken_--;
    } else {
      throw std::runtime_error("no cards played");
    }
  }
}

Cards Game::valid_plays_pruned() const {
  return card_normalizer_.prune_equivalent(valid_plays_all());
}

Cards Game::valid_plays_all() const {
  if (finished()) {
    return Cards();
  }

  Cards        c = hands_.hand(next_seat_);
  const Trick &t = current_trick();
  if (t.started()) {
    Cards cs = c.intersect(t.lead_suit());
    if (!cs.empty()) {
      return cs;
    }
  }
  return c;
}

const Game::State &Game::normalized_state() const {
  assert(start_of_trick());
  auto &cached = state_stack_[tricks_taken_];
  if (!cached.has_value()) {
    cached.emplace(
        card_normalizer_.normalize(hands_.hand(WEST)),
        card_normalizer_.normalize(hands_.hand(NORTH)),
        card_normalizer_.normalize(hands_.hand(EAST)),
        card_normalizer_.normalize(hands_.hand(SOUTH)),
        next_seat_
    );
  }
  return *cached;
}

const Hands &Game::normalized_hands() const {
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