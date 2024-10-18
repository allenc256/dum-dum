#include "mini_solver.h"

static void update_value(
    TpnTableValue       &value,
    const TpnTableValue &child_value,
    const Card          &play,
    bool                 maximizing
) {
  if (maximizing) {
    bool updated = value.update_lower_bound(child_value.lower_bound());
    if (updated && value.has_tight_bounds()) {
      value.update_pv_play(play);
    }
  } else {
    bool updated = value.update_upper_bound(child_value.upper_bound());
    if (updated && value.has_tight_bounds()) {
      value.update_pv_play(play);
    }
  }
}

TpnTableValue MiniSolver::compute_value(int max_depth) {
  assert(game_.start_of_trick());

  if (game_.finished() || game_.tricks_taken() >= max_depth) {
    // TODO: compute evaluation here for non-terminal
    return TpnTableValue(
        game_.tricks_taken_by_ns(), game_.tricks_taken_by_ns(), max_depth
    );
  }

  auto [found, value] = tpn_table_.lookup_value(max_depth);
  if (found) {
    return value;
  }

  Seat me         = game_.next_seat();
  Seat left       = game_.next_seat(1);
  Seat partner    = game_.next_seat(2);
  Seat right      = game_.next_seat(3);
  bool maximizing = me == NORTH || me == SOUTH;

  Cards left_trumps;
  Cards right_trumps;
  Cards partner_trumps;
  if (game_.trump_suit() != NO_TRUMP) {
    left_trumps    = game_.hand(left).intersect(game_.trump_suit());
    right_trumps   = game_.hand(right).intersect(game_.trump_suit());
    partner_trumps = game_.hand(partner).intersect(game_.trump_suit());
  }

  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    Cards my_cards       = game_.hand(me).intersect(suit);
    Cards left_cards     = game_.hand(left).intersect(suit);
    Cards right_cards    = game_.hand(right).intersect(suit);
    Cards partner_cards  = game_.hand(partner).intersect(suit);
    bool  left_can_ruff  = left_cards.empty() && !left_trumps.empty();
    bool  right_can_ruff = right_cards.empty() && !right_trumps.empty();

    if (left_can_ruff || right_can_ruff || my_cards.empty()) {
      continue;
    }

    Cards poss_winners;
    auto  it = left_cards.union_with(right_cards).iter_highest();
    if (it.valid()) {
      poss_winners = Cards::higher_ranking(it.card());
    } else {
      poss_winners = Cards::all(suit);
    }

    Cards my_poss_winners      = my_cards.intersect(poss_winners);
    Cards partner_poss_winners = partner_cards.intersect(poss_winners);

    for (auto it = my_poss_winners.iter_highest();
         it.valid() && !value.has_tight_bounds();
         it = my_poss_winners.iter_lower(it)) {
      Card my_play = it.card();
      game_.play(my_play);
      play_opp_lowest();
      play_partner_lowest();
      play_opp_lowest();
      assert(game_.next_seat() == me || game_.next_seat() == partner);
      update_value(value, compute_value(max_depth), my_play, maximizing);
      game_.unplay();
      game_.unplay();
      game_.unplay();
      game_.unplay();
    }

    for (auto it = partner_poss_winners.iter_highest();
         it.valid() && !value.has_tight_bounds();
         it = partner_poss_winners.iter_lower(it)) {
      Card my_play = play_my_lowest(suit);
      play_opp_lowest();
      game_.play(it.card());
      play_opp_lowest();
      assert(game_.next_seat() == me || game_.next_seat() == partner);
      update_value(value, compute_value(max_depth), my_play, maximizing);
      game_.unplay();
      game_.unplay();
      game_.unplay();
      game_.unplay();
    }

    if (partner_cards.empty() && !partner_trumps.empty() &&
        !value.has_tight_bounds()) {
      Card my_play = play_my_lowest(suit);
      play_opp_lowest();
      play_partner_ruff();
      play_opp_lowest();
      assert(game_.next_seat() == me || game_.next_seat() == partner);
      update_value(value, compute_value(max_depth), my_play, maximizing);
      game_.unplay();
      game_.unplay();
      game_.unplay();
      game_.unplay();
    }

    // TODO:
    // - overruffs?
    // - finesses?
  }

  tpn_table_.update_value(value);

  return value;
}

Card MiniSolver::play_my_lowest(Suit suit) {
  auto it = game_.valid_plays().intersect(suit).iter_lowest();
  assert(it.valid());
  Card my_play = it.card();
  game_.play(my_play);
  return my_play;
}

void MiniSolver::play_partner_lowest() {
  Cards valid_plays = game_.valid_plays();
  auto  it          = valid_plays.iter_lowest();
  assert(it.valid());
  game_.play(it.card());
}

void MiniSolver::play_partner_ruff() {
  assert(game_.trump_suit() != NO_TRUMP);
  Cards ruffs = game_.valid_plays().intersect(game_.trump_suit());
  assert(!ruffs.empty());
  game_.play(ruffs.iter_lowest().card());
}

void MiniSolver::play_opp_lowest() {
  Suit  suit          = game_.current_trick().lead_suit();
  Cards cards_in_suit = game_.hand(game_.next_seat()).intersect(suit);
  if (cards_in_suit.empty()) {
    game_.play_null();
  } else {
    game_.play(cards_in_suit.iter_lowest().card());
  }
}
