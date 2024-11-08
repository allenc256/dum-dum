#pragma once

#include <array>
#include <iostream>
#include <optional>
#include <random>

#include "card_model.h"

enum Seat : int {
  WEST,
  NORTH,
  EAST,
  SOUTH,
};

constexpr Seat FIRST_SEAT = WEST;
constexpr Seat LAST_SEAT  = SOUTH;

inline Seat operator++(Seat &s, int) { return (Seat)((int &)s)++; }
inline Seat operator--(Seat &s, int) { return (Seat)((int &)s)--; }

inline Seat left_seat(Seat s) { return (Seat)((s + 3) & 3); }
inline Seat right_seat(Seat s) { return (Seat)((s + 1) & 3); }

inline Seat left_seat(Seat s, int i) {
  assert(i >= 0);
  return (Seat)((s + i * 3) & 3);
}

inline Seat right_seat(Seat s, int i) {
  assert(i >= 0);
  return (Seat)((s + i) & 3);
}

std::ostream &operator<<(std::ostream &os, Seat s);
std::istream &operator>>(std::istream &is, Seat &s);

class Hands {
public:
  Hands() {}
  Hands(Cards w, Cards n, Cards e, Cards s) : hands_{w, n, e, s} {}

  Cards hand(Seat seat) const { return hands_[seat]; }
  void  add_card(Seat seat, Card card) { hands_[seat].add(card); }
  void  remove_card(Seat seat, Card card) { hands_[seat].remove(card); }

  bool all_same_size() const {
    int size = hands_[0].count();
    for (int i = 1; i < 4; i++) {
      if (hands_[i].count() != size) {
        return false;
      }
    }
    return true;
  }

  bool all_disjoint() const {
    for (int i = 0; i < 4; i++) {
      for (int j = i + 1; j < 4; j++) {
        if (!hands_[i].disjoint(hands_[j])) {
          return false;
        }
      }
    }
    return true;
  }

  Cards all_cards() const {
    Cards c;
    for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
      c.add_all(hands_[seat]);
    }
    return c;
  }

  template <typename H> friend H AbslHashValue(H h, const Hands &hands) {
    return H::combine(std::move(h), hands.hands_);
  }

  bool operator==(const Hands &hands) const = default;

  void pretty_print(std::ostream &os, Cards winner_by_rank) const;

private:
  std::array<Cards, 4> hands_;
};

class Trick {
public:
  Trick()
      : trump_suit_(NO_TRUMP),
        lead_seat_(WEST),
        lead_suit_(NO_TRUMP),
        card_count_(0) {}

  bool has_card(int index) const {
    assert(index >= 0 && index < card_count_);
    return cards_[index].has_value();
  }

  Card card(int index) const {
    assert(has_card(index));
    return *cards_[index];
  }

  int  card_count() const { return card_count_; }
  bool started() const { return card_count_ > 0; }
  bool finished() const { return card_count_ >= 4; }

  Seat lead_seat() const {
    assert(started());
    return lead_seat_;
  }

  Suit lead_suit() const {
    assert(started());
    return lead_suit_;
  }

  Suit trump_suit() const {
    assert(started());
    return trump_suit_;
  }

  Seat seat(int index) const {
    assert(index >= 0 && index < 4);
    return right_seat(lead_seat_, index);
  }

  Seat next_seat() const {
    assert(started() && !finished());
    return right_seat(lead_seat_, card_count_);
  }

  Seat winning_seat() const {
    assert(started());
    return right_seat(lead_seat_, winning_index());
  }

  Card winning_card() const {
    assert(started());
    int i = winning_index();
    assert(cards_[i].has_value());
    return *cards_[i];
  }

  int winning_index() const {
    assert(started());
    return winning_index_[card_count_ - 1];
  }

  Cards valid_plays(Cards hand) const {
    assert(!finished());
    if (!started()) {
      return hand;
    }
    Cards c = hand.intersect(lead_suit_);
    return c.empty() ? hand : c;
  }

  Cards winning_cards() const {
    assert(!finished());
    return card_count_ > 0 ? winning_cards_[card_count_ - 1] : Cards::all();
  }

  void play_start(Suit trump_suit, Seat lead_seat, Card c) {
    assert(card_count_ == 0);
    trump_suit_       = trump_suit;
    lead_seat_        = lead_seat;
    lead_suit_        = c.suit();
    cards_[0]         = c;
    card_count_       = 1;
    winning_cards_[0] = higher_cards(c);
    winning_index_[0] = 0;
  }

  void play_continue(Card c) {
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

  void play_null() {
    assert(card_count_ > 0 && card_count_ < 4);
    winning_index_[card_count_] = winning_index();
    winning_cards_[card_count_] = winning_cards();
    cards_[card_count_]         = std::nullopt;
    card_count_++;
  }

  std::optional<Card> unplay() {
    assert(card_count_ > 0);
    card_count_--;
    return cards_[card_count_];
  }

  Cards higher_cards(Card w) const {
    assert(started());
    if (trump_suit_ == NO_TRUMP || w.suit() == trump_suit_) {
      return Cards::higher_ranking(w);
    } else {
      return Cards::higher_ranking(w).with_all(Cards::all(trump_suit_));
    }
  }

  Card highest_card(Cards hand) const {
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

  bool is_higher_card(Card c1, Card c2) const {
    return card_value(c1) > card_value(c2);
  }

  Card higher_card(Card c1, Card c2) const {
    return is_higher_card(c1, c2) ? c1 : c2;
  }

  Cards all_cards() const {
    Cards cards;
    for (int i = 0; i < card_count_; i++) {
      if (has_card(i)) {
        cards.add(card(i));
      }
    }
    return cards;
  }

  Cards winners_by_rank(const Hands &hands) const {
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

private:
  bool won_by_rank() const {
    Card w = winning_card();
    for (int i = 0; i < 4; i++) {
      if (has_card(i)) {
        Card c = card(i);
        if (c.suit() == w.suit() && c.rank() < w.rank()) {
          return true;
        }
      }
    }
    return false;
  }

  int card_value(Card c) const {
    if (c.suit() == trump_suit_) {
      return c.rank() + 14;
    } else if (c.suit() == lead_suit_) {
      return c.rank() + 1;
    } else {
      return 0;
    }
  }

  Suit                trump_suit_;
  Seat                lead_seat_;
  Suit                lead_suit_;
  std::optional<Card> cards_[4];
  int                 card_count_;
  int                 winning_index_[4];
  Cards               winning_cards_[4];
};

std::ostream &operator<<(std::ostream &os, const Trick &t);

class Game {
public:
  struct State {
    Hands normalized_hands;
    Seat  next_seat;

    State() {}

    State(Cards w, Cards n, Cards e, Cards s, Seat next_seat)
        : normalized_hands(w, n, e, s),
          next_seat(next_seat) {}

    template <typename H> friend H AbslHashValue(H h, const State &state) {
      return H::combine(std::move(h), state.normalized_hands, state.next_seat);
    }

    bool operator==(const State &state) const = default;
  };

  Game(Suit trump_suit, Seat first_lead_seat, const Hands &hands);

  Suit         trump_suit() const { return trump_suit_; }
  Seat         lead_seat() const { return lead_seat_; }
  Cards        hand(Seat seat) const { return hands_.hand(seat); }
  const Hands &hands() const { return hands_; }
  Seat         next_seat() const { return next_seat_; }
  Seat         next_seat(int i) const { return right_seat(next_seat_, i); }

  Trick       &current_trick() { return tricks_[tricks_taken_]; }
  const Trick &current_trick() const { return tricks_[tricks_taken_]; }

  const Trick &last_trick() const {
    assert(tricks_taken_ > 0);
    return tricks_[tricks_taken_ - 1];
  }

  const Trick &trick(int i) const {
    assert(i < tricks_taken_);
    return tricks_[i];
  }

  bool started() const {
    return current_trick().started() || tricks_taken_ > 0;
  }

  int tricks_taken() const { return tricks_taken_; }
  int tricks_left() const { return tricks_max_ - tricks_taken_; }
  int tricks_max() const { return tricks_max_; }
  int tricks_taken_by_ns() const { return tricks_taken_by_ns_; }
  int tricks_taken_by_ew() const { return tricks_taken_ - tricks_taken_by_ns_; }
  bool finished() const { return tricks_taken_ == tricks_max_; }
  bool start_of_trick() const { return !current_trick().started(); }

  void play(Card card);
  void play_null();
  void unplay();

  bool  valid_play(Card c) const;
  Cards valid_plays_pruned() const;
  Cards valid_plays_all() const;

  const State &normalized_state() const;

  Card normalize_card(Card card) const {
    return card_normalizer_.normalize(card);
  }

  Card denormalize_card(Card card) const {
    return card_normalizer_.denormalize(card);
  }

  void pretty_print(std::ostream &os) const;

private:
  void finish_play();

  using StateStack = std::array<std::optional<State>, 14>;

  Hands              hands_;
  Suit               trump_suit_;
  Seat               lead_seat_;
  Seat               next_seat_;
  Trick              tricks_[14];
  int                tricks_taken_;
  int                tricks_max_;
  int                tricks_taken_by_ns_;
  CardNormalizer     card_normalizer_;
  mutable StateStack state_stack_;
};

std::ostream &operator<<(std::ostream &os, const Game &g);
