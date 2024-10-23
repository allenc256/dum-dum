#include "tpn_table.h"

// static Rank next_lower_rank_in_suit(Cards cards, Rank rank, Suit suit) {
//   auto it = cards.intersect(suit).iter_lower(Card(rank, suit));
//   if (it.valid()) {
//     return it.card().rank();
//   } else {
//     return RANK_2;
//   }
// }

AbsLevel::AbsLevel(const Hands &hands, const Trick &trick) : AbsLevel() {
  assert(trick.finished());

  if (!trick.won_by_rank()) {
    return;
  }

  Cards removed_cards =
      hands.all_cards().union_with(trick.all_cards()).complement();
  Card  winning_card = trick.winning_card();
  Cards winning_hand = hands.hand(trick.winning_seat()).with(winning_card);
  Rank  winning_rank =
      winning_hand.lowest_equivalent(winning_card, removed_cards).rank();
  Rank rank_cutoff = (Rank)(winning_rank - 1);

  // Cards all_cards    = hands.all_cards().union_with(trick.all_cards());
  // Cards all_removed  = all_cards.complement();
  // Card  winning_card = trick.winning_card();
  // Cards winning_hand = hands.hand(trick.winning_seat()).with(winning_card);
  // Rank  winning_rank =
  //     winning_hand.lowest_equivalent(winning_card, all_removed).rank();
  // Rank rank_cutoff =
  //     next_lower_rank_in_suit(all_cards, winning_rank, winning_card.suit());

  rank_cutoffs_[winning_card.suit()] = rank_cutoff;
}

std::ostream &operator<<(std::ostream &os, const AbsLevel &level) {
  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    Rank rank = level.rank_cutoff(suit);
    if (rank < ACE) {
      os << (Rank)(rank + 1);
    } else {
      os << '-';
    }
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const AbsState &s) {
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
      auto &ss         = s.suit_states_[suit];
      Cards high_cards = ss.high_cards(seat, suit);
      int   low_cards  = ss.low_cards(seat);

      if (high_cards.empty() && low_cards == 0) {
        continue;
      }

      os << suit;
      for (auto it = high_cards.iter_highest(); it.valid();
           it      = high_cards.iter_lower(it)) {
        os << it.card().rank();
      }
      for (int i = 0; i < ss.low_cards(seat); i++) {
        os << 'X';
      }
    }
    os << '/';
  }
  os << s.lead_seat_;
  return os;
}

bool TpnTable2::lookup_value(int alpha, int beta, int max_depth, Value &value) {
  int  normed_alpha = alpha - game_.tricks_taken_by_ns();
  int  normed_beta  = beta - game_.tricks_taken_by_ns();
  bool found = lookup_value_normed(normed_alpha, normed_beta, max_depth, value);
  denorm_value(value);
  return found;
}

void TpnTable2::upsert_value(int max_depth, const Value &value) {
  Value normed = value;
  norm_value(normed);
  upsert_value_normed(max_depth, normed);
}

bool TpnTable2::lookup_value_normed(
    int alpha, int beta, int max_depth, Value &value
) const {
  value.level       = {};
  value.lower_bound = 0;
  value.upper_bound = to_int8_t(game_.tricks_left());
  value.pv_play     = std::nullopt;

  SeatShapes key(game_);
  auto       range = multimap_.equal_range(key);
  for (auto i = range.first; i != range.second; ++i) {
    auto &entry = i->second;
    if (entry.state.matches(game_) && entry.max_depth >= max_depth) {
      if (entry.lower_bound == entry.upper_bound) {
        value.level       = entry.state.level();
        value.lower_bound = entry.lower_bound;
        value.upper_bound = entry.upper_bound;
        value.pv_play     = choose_pv_play(entry);
        return true;
      }

      if (entry.lower_bound > value.lower_bound) {
        value.lower_bound = entry.lower_bound;
        value.level.intersect(entry.state.level());
        if (value.lower_bound >= beta) {
          return true;
        }
      }

      if (entry.upper_bound < value.upper_bound) {
        value.upper_bound = entry.upper_bound;
        value.level.intersect(entry.state.level());
        if (value.upper_bound <= alpha) {
          return true;
        }
      }
    }
  }

  return false;
}

Card TpnTable2::choose_pv_play(const Entry &entry) const {
  if (entry.pv_play_type == HIGH_CARD) {
    return entry.pv_play_card;
  }

  assert(entry.pv_play_type == LOW_CARD);

  Suit  suit        = entry.pv_play_card.suit();
  Rank  rank_cutoff = entry.state.level().rank_cutoff(suit);
  Cards low_cards =
      game_.hand(game_.next_seat())
          .intersect(suit)
          .subtract(Cards::higher_ranking(Card(rank_cutoff, suit)));

  assert(!low_cards.empty());
  return low_cards.iter_lowest().card();
}

void TpnTable2::upsert_value_normed(int max_depth, const Value &value) {
  AbsState   state = {value.level, game_};
  SeatShapes key   = {game_};
  auto       range = multimap_.equal_range(key);
  PlayType   pv_play_type;
  Card       pv_play_card;

  if (value.pv_play.has_value()) {
    pv_play_type =
        value.level.is_high_card(*value.pv_play) ? HIGH_CARD : LOW_CARD;
    pv_play_card = *value.pv_play;
  } else {
    pv_play_type = UNKNOWN;
  }

  // std::cout << "put: " << state << " -> " << (int)value.lower_bound << " "
  //           << (int)value.upper_bound << std::endl;

  for (auto i = range.first; i != range.second; ++i) {
    Entry &entry = i->second;
    if (state == entry.state) {
      entry.lower_bound  = value.lower_bound;
      entry.upper_bound  = value.upper_bound;
      entry.pv_play_type = pv_play_type;
      entry.pv_play_card = pv_play_card;
      entry.max_depth    = to_int8_t(max_depth);
      return;
    }
  }

  multimap_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(key),
      std::forward_as_tuple(
          state,
          value.lower_bound,
          value.upper_bound,
          max_depth,
          pv_play_type,
          pv_play_card
      )
  );
}

void TpnTable2::norm_value(Value &value) {
  // value.level.normalize(game_);
  value.lower_bound -= game_.tricks_taken_by_ns();
  value.upper_bound -= game_.tricks_taken_by_ns();
  // if (value.pv_play.has_value()) {
  //   value.pv_play = game_.normalize_card(*value.pv_play);
  // }
}

void TpnTable2::denorm_value(Value &value) {
  // value.level.denormalize(game_);
  value.lower_bound += game_.tricks_taken_by_ns();
  value.upper_bound += game_.tricks_taken_by_ns();
  // if (value.pv_play.has_value()) {
  //   value.pv_play = game_.denormalize_card(*value.pv_play);
  // }
}
