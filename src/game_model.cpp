#include "game_model.h"

Seat operator++(Seat &s, int) { return (Seat)((int &)s)++; }
Seat operator--(Seat &s, int) { return (Seat)((int &)s)--; }

Seat left_seat(Seat s) { return (Seat)((s + 3) & 3); }
Seat right_seat(Seat s) { return (Seat)((s + 1) & 3); }

Seat left_seat(Seat s, int i) {
  assert(i >= 0);
  return (Seat)((s + i * 3) & 3);
}

Seat right_seat(Seat s, int i) {
  assert(i >= 0);
  return (Seat)((s + i) & 3);
}

static constexpr std::string_view SEAT_CHARS = "WNES";

Seat parse_seat(Parser &parser) {
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    if (parser.try_parse(SEAT_CHARS[seat])) {
      return seat;
    }
  }
  throw parser.error("expected seat");
}

Seat parse_seat(std::string_view str) {
  Parser parser(str);
  return parse_seat(parser);
}

std::string_view std::formatter<Seat>::to_string(Seat seat) const {
  return SEAT_CHARS.substr(seat, 1);
}

Hands::Hands(Parser &parser) {
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    if (seat != FIRST_SEAT) {
      if (!parser.try_parse('/')) {
        throw parser.error("expected '/' delimiter");
      }
    }
    hands_[seat] = Cards(parser);
  }
}

Hands::Hands(Cards w, Cards n, Cards e, Cards s) : hands_{w, n, e, s} {}

Hands::Hands(std::string_view s) {
  Parser parser(s);
  *this = Hands(parser);
}

Hands Hands::make_partition(Cards winners_by_rank) const {
  return Hands(
      hands_[0].intersect(winners_by_rank),
      hands_[1].intersect(winners_by_rank),
      hands_[2].intersect(winners_by_rank),
      hands_[3].intersect(winners_by_rank)
  );
}

Cards Hands::hand(Seat seat) const { return hands_[seat]; }
void  Hands::add_card(Seat seat, Card card) { hands_[seat].add(card); }
void  Hands::remove_card(Seat seat, Card card) { hands_[seat].remove(card); }

bool Hands::all_same_size() const {
  int size = hands_[0].count();
  for (int i = 1; i < 4; i++) {
    if (hands_[i].count() != size) {
      return false;
    }
  }
  return true;
}

bool Hands::all_disjoint() const {
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      if (!hands_[i].disjoint(hands_[j])) {
        return false;
      }
    }
  }
  return true;
}

Cards Hands::all_cards() const {
  Cards c;
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    c.add_all(hands_[seat]);
  }
  return c;
}

bool Hands::contains_all(const Hands &other) const {
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    if (!hands_[seat].contains_all(other.hands_[seat])) {
      return false;
    }
  }
  return true;
}

Hands Hands::normalize() const {
  Cards removed = all_cards().complement();
  return Hands(
      hands_[0].normalize(removed),
      hands_[1].normalize(removed),
      hands_[2].normalize(removed),
      hands_[3].normalize(removed)
  );
}

Trick::Trick()
    : trump_suit_(NO_TRUMP),
      lead_seat_(WEST),
      lead_suit_(NO_TRUMP),
      card_count_(0) {}

Card Trick::card(int index) const { return cards_[index]; }
int  Trick::card_count() const { return card_count_; }
bool Trick::started() const { return card_count_ > 0; }
bool Trick::finished() const { return card_count_ >= 4; }

Seat Trick::lead_seat() const {
  assert(started());
  return lead_seat_;
}

Suit Trick::lead_suit() const {
  assert(started());
  return lead_suit_;
}

Suit Trick::trump_suit() const {
  assert(started());
  return trump_suit_;
}

Seat Trick::seat(int index) const {
  assert(index >= 0 && index < 4);
  return right_seat(lead_seat_, index);
}

Seat Trick::next_seat() const {
  assert(started() && !finished());
  return right_seat(lead_seat_, card_count_);
}

Seat Trick::winning_seat() const {
  assert(started());
  return right_seat(lead_seat_, winning_index());
}

Card Trick::winning_card() const {
  assert(started());
  return cards_[winning_index()];
}

int Trick::winning_index() const {
  assert(started());
  return winning_index_[card_count_ - 1];
}

Cards Trick::valid_plays(Cards hand) const {
  assert(!finished());
  if (!started()) {
    return hand;
  }
  Cards c = hand.intersect(lead_suit_);
  return c.empty() ? hand : c;
}

Cards Trick::winning_cards() const {
  assert(!finished());
  return card_count_ > 0 ? winning_cards_[card_count_ - 1] : Cards::all();
}

void Trick::play_start(Suit trump_suit, Seat lead_seat, Card c) {
  assert(card_count_ == 0);
  trump_suit_       = trump_suit;
  lead_seat_        = lead_seat;
  lead_suit_        = c.suit();
  cards_[0]         = c;
  card_count_       = 1;
  winning_cards_[0] = higher_cards(c);
  winning_index_[0] = 0;
}

void Trick::play_continue(Card c) {
  assert(card_count_ > 0 && card_count_ < 4);
  if (winning_cards().contains(c)) {
    winning_index_[card_count_] = card_count_;
    winning_cards_[card_count_] = higher_cards(c);
  } else {
    winning_index_[card_count_] = winning_index();
    winning_cards_[card_count_] = winning_cards();
  }
  cards_[card_count_] = c;
  card_count_++;
}

Card Trick::unplay() {
  assert(card_count_ > 0);
  card_count_--;
  return cards_[card_count_];
}

Cards Trick::higher_cards(Card w) const {
  assert(started());
  if (trump_suit_ == NO_TRUMP || w.suit() == trump_suit_) {
    return Cards::higher_ranking(w);
  } else {
    return Cards::higher_ranking(w).with_all(Cards::all(trump_suit_));
  }
}

Card Trick::highest_card(Cards hand) const {
  assert(started());
  Cards in_suit = hand.intersect(lead_suit_);
  if (!in_suit.empty()) {
    return in_suit.highest();
  }
  Cards trumps = hand.intersect(trump_suit_);
  if (!trumps.empty()) {
    return trumps.highest();
  }
  return hand.highest();
}

bool Trick::is_higher_card(Card c1, Card c2) const {
  return card_value(c1) > card_value(c2);
}

Card Trick::higher_card(Card c1, Card c2) const {
  return is_higher_card(c1, c2) ? c1 : c2;
}

Cards Trick::all_cards() const {
  Cards cards;
  for (int i = 0; i < card_count_; i++) {
    cards.add(card(i));
  }
  return cards;
}

Cards Trick::winners_by_rank(const Hands &hands) const {
  assert(finished());
  if (!won_by_rank()) {
    return Cards();
  }

  Card  w_card  = winning_card();
  Cards w_hand  = hands.hand(winning_seat());
  Cards removed = hands.all_cards().with_all(all_cards()).complement();

  w_card = w_hand.lowest_equivalent(w_card, removed);

  return Cards::higher_ranking_or_eq(w_card);
}

bool Trick::won_by_rank() const {
  Card w = winning_card();
  for (int i = 0; i < 4; i++) {
    Card c = card(i);
    if (c.suit() == w.suit() && c.rank() < w.rank()) {
      return true;
    }
  }
  return false;
}

int Trick::card_value(Card c) const {
  if (c.suit() == trump_suit_) {
    return c.rank() + 14;
  } else if (c.suit() == lead_suit_) {
    return c.rank() + 1;
  } else {
    return 0;
  }
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

Suit         Game::trump_suit() const { return trump_suit_; }
Seat         Game::lead_seat() const { return lead_seat_; }
Cards        Game::hand(Seat seat) const { return hands_.hand(seat); }
const Hands &Game::hands() const { return hands_; }
Seat         Game::next_seat() const { return next_seat_; }
Seat         Game::next_seat(int i) const { return right_seat(next_seat_, i); }

Trick       &Game::current_trick() { return tricks_[tricks_taken_]; }
const Trick &Game::current_trick() const { return tricks_[tricks_taken_]; }

const Trick &Game::last_trick() const {
  assert(tricks_taken_ > 0);
  return tricks_[tricks_taken_ - 1];
}

const Trick &Game::trick(int i) const {
  assert(i < tricks_taken_);
  return tricks_[i];
}

bool Game::started() const {
  return current_trick().started() || tricks_taken_ > 0;
}

int Game::tricks_taken() const { return tricks_taken_; }
int Game::tricks_left() const { return tricks_max_ - tricks_taken_; }
int Game::tricks_max() const { return tricks_max_; }
int Game::tricks_taken_by_ns() const { return tricks_taken_by_ns_; }
int Game::tricks_taken_by_ew() const {
  return tricks_taken_ - tricks_taken_by_ns_;
}
bool Game::finished() const { return tricks_taken_ == tricks_max_; }
bool Game::start_of_trick() const { return !current_trick().started(); }

Card Game::normalize_card(Card card) const {
  return card_normalizer_.normalize(card);
}

Cards Game::normalize_wbr(Cards winners_by_rank) const {
  return card_normalizer_.normalize_wbr(winners_by_rank);
}

Cards Game::denormalize_wbr(Cards winners_by_rank) const {
  return card_normalizer_.denormalize_wbr(winners_by_rank);
}

Card Game::denormalize_card(Card card) const {
  return card_normalizer_.denormalize(card);
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

  if (t.finished()) {
    next_seat_ = t.winning_seat();
    if (next_seat_ == NORTH || next_seat_ == SOUTH) {
      tricks_taken_by_ns_++;
    }
    for (int i = 0; i < 4; i++) {
      card_normalizer_.remove(t.card(i));
    }
    tricks_taken_++;
    assert(tricks_taken_ <= 13);
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
        card_normalizer_.add(t.card(i));
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
