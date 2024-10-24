#include "tpn_table.h"

std::ostream &operator<<(std::ostream &os, const SeatShape &shape) {
  return os << std::hex << std::setfill('0') << std::setw(4) << shape.bits_
            << std::dec;
}

std::ostream &operator<<(std::ostream &os, const SeatShapes &shapes) {
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    os << shapes.seat_shapes_[seat];
  }
  return os;
}

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

  rank_cutoffs_[winning_card.suit()] = rank_cutoff;
}

static Rank norm_rank_cutoff(const Game &game, Rank rank_cutoff, Suit suit) {
  if (rank_cutoff == ACE) {
    return rank_cutoff;
  }
  Card c = Card((Rank)(rank_cutoff + 1), suit);
  return (Rank)(game.normalize_card(c).rank() - 1);
}

static Rank denorm_rank_cutoff(const Game &game, Rank rank_cutoff, Suit suit) {
  if (rank_cutoff == ACE) {
    return rank_cutoff;
  }
  Card c = Card((Rank)(rank_cutoff + 1), suit);
  return (Rank)(game.denormalize_card(c).rank() - 1);
}

void AbsLevel::normalize(Game &game) {
  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    rank_cutoffs_[suit] = norm_rank_cutoff(game, rank_cutoffs_[suit], suit);
  }
}

void AbsLevel::denormalize(Game &game) {
  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    rank_cutoffs_[suit] = denorm_rank_cutoff(game, rank_cutoffs_[suit], suit);
  }
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

bool TpnTable::lookup_value(int alpha, int beta, int max_depth, Value &value) {
  int  normed_alpha = alpha - game_.tricks_taken_by_ns();
  int  normed_beta  = beta - game_.tricks_taken_by_ns();
  bool found = lookup_value_normed(normed_alpha, normed_beta, max_depth, value);
  denorm_value(value);
  return found;
}

void TpnTable::upsert_value(int max_depth, const Value &value) {
  Value normed = value;
  norm_value(normed);
  upsert_value_normed(max_depth, normed);
}

bool TpnTable::lookup_value_normed(
    int alpha, int beta, int max_depth, Value &value
) {
  value.level       = {};
  value.lower_bound = 0;
  value.upper_bound = to_int8_t(game_.tricks_left());
  value.pv_play     = std::nullopt;

  SeatShapes key   = {game_};
  auto       range = multimap_.equal_range(key);
  bool       found = false;

  for (auto i = range.first; i != range.second; ++i) {
    Entry &entry = i->second;
    stats_.lookup_entries_examined++;
    if (entry.state.matches(game_) && entry.max_depth >= max_depth) {
      found = true;

      if (entry.lower_bound == entry.upper_bound) {
        value.level       = entry.state.level();
        value.lower_bound = entry.lower_bound;
        value.upper_bound = entry.upper_bound;
        value.pv_play     = entry.pv_play;
        stats_.lookup_hits++;
        return true;
      }

      if (entry.lower_bound > value.lower_bound) {
        value.lower_bound = entry.lower_bound;
        value.level.intersect(entry.state.level());
        if (value.lower_bound >= beta) {
          stats_.lookup_hits++;
          return true;
        }
      }

      if (entry.upper_bound < value.upper_bound) {
        value.upper_bound = entry.upper_bound;
        value.level.intersect(entry.state.level());
        if (value.upper_bound <= alpha) {
          stats_.lookup_hits++;
          return true;
        }
      }
    }
  }

  stats_.lookup_misses++;
  return found;
}

void TpnTable::upsert_value_normed(int max_depth, const Value &value) {
  AbsState   state    = {value.level, game_};
  SeatShapes key      = {game_};
  auto       range    = multimap_.equal_range(key);
  int        examined = 0;

  for (auto i = range.first; i != range.second; ++i) {
    Entry &entry = i->second;
    examined++;
    if (entry.state == state) {
      entry.lower_bound = value.lower_bound;
      entry.upper_bound = value.upper_bound;
      entry.max_depth   = to_int8_t(max_depth);
      entry.pv_play     = value.pv_play;
      stats_.upsert_hits++;
      stats_.upsert_entries_examined += examined;
      return;
    }
  }

  auto it = multimap_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(key),
      std::forward_as_tuple()
  );
  Entry &entry      = it->second;
  entry.state       = state;
  entry.lower_bound = value.lower_bound;
  entry.upper_bound = value.upper_bound;
  entry.max_depth   = to_int8_t(max_depth);
  entry.pv_play     = value.pv_play;

  stats_.upsert_misses++;
  stats_.upsert_entries_examined += examined;
  int64_t bucket_size    = examined + 1;
  stats_.max_bucket_size = std::max(stats_.max_bucket_size, bucket_size);
}

void TpnTable::norm_value(Value &value) {
  value.lower_bound -= game_.tricks_taken_by_ns();
  value.upper_bound -= game_.tricks_taken_by_ns();
  if (value.pv_play.has_value()) {
    if (value.level.is_high_card(*value.pv_play)) {
      value.pv_play = game_.normalize_card(*value.pv_play);
    } else {
      value.pv_play = Card(RANK_2, value.pv_play->suit());
    }
  }
  value.level.normalize(game_);
}

void TpnTable::denorm_value(Value &value) {
  value.lower_bound += game_.tricks_taken_by_ns();
  value.upper_bound += game_.tricks_taken_by_ns();
  if (value.pv_play.has_value()) {
    if (value.level.is_high_card(*value.pv_play)) {
      value.pv_play = game_.denormalize_card(*value.pv_play);
    } else {
      assert(value.pv_play->rank() == RANK_2);
      Cards valid_plays =
          game_.hand(game_.next_seat()).intersect(value.pv_play->suit());
      assert(!valid_plays.empty());
      value.pv_play = valid_plays.iter_lowest().card();
    }
  }
  value.level.denormalize(game_);
}
