#include "mini_solver.h"

static void update_bounds(
    Bounds       &bounds,
    const Bounds &child_bounds,
    const Card   &play,
    bool          maximizing
) {
  if (maximizing) {
    bounds.lower     = std::max(bounds.lower, (int8_t)(child_bounds.lower + 1));
    bounds.best_play = play;
  } else {
    bounds.upper     = std::min(bounds.upper, child_bounds.upper);
    bounds.best_play = play;
  }
  assert(bounds.lower <= bounds.upper);
}

Bounds MiniSolver::compute_bounds(int max_depth) {
  if (game_.finished()) {
    return {.lower = 0, .upper = 0, .max_depth = (int8_t)max_depth};
  }

  assert(game_.start_of_trick());

  Cards ignorable_cards = game_.ignorable_cards();
  auto  it              = tpn_table_.find(game_.game_state());
  if (it != tpn_table_.end() && it->second.max_depth == max_depth) {
    return it->second;
  }

  Bounds bounds = {
      .lower     = 0,
      .upper     = (int8_t)game_.tricks_left(),
      .max_depth = (int8_t)max_depth
  };

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

    Cards my_poss_winners =
        my_cards.intersect(poss_winners).prune_equivalent(ignorable_cards);
    Cards partner_poss_winners =
        partner_cards.intersect(poss_winners).prune_equivalent(ignorable_cards);

    for (auto it = my_poss_winners.iter_highest();
         it.valid() && bounds.lower < bounds.upper;
         it = my_poss_winners.iter_lower(it)) {
      Card my_play = it.card();
      game_.play(my_play);
      play_opp_lowest();
      play_partner_lowest();
      play_opp_lowest();
      assert(game_.next_seat() == me || game_.next_seat() == partner);
      update_bounds(bounds, compute_bounds(max_depth), my_play, maximizing);
      game_.unplay();
      game_.unplay();
      game_.unplay();
      game_.unplay();
    }

    for (auto it = partner_poss_winners.iter_highest();
         it.valid() && bounds.lower < bounds.upper;
         it = partner_poss_winners.iter_lower(it)) {
      Card my_play = play_my_lowest(suit);
      play_opp_lowest();
      game_.play(it.card());
      play_opp_lowest();
      assert(game_.next_seat() == me || game_.next_seat() == partner);
      update_bounds(bounds, compute_bounds(max_depth), my_play, maximizing);
      game_.unplay();
      game_.unplay();
      game_.unplay();
      game_.unplay();
    }

    if (partner_cards.empty() && !partner_trumps.empty() &&
        bounds.lower < bounds.upper) {
      Card my_play = play_my_lowest(suit);
      play_opp_lowest();
      play_partner_ruff();
      play_opp_lowest();
      assert(game_.next_seat() == me || game_.next_seat() == partner);
      update_bounds(bounds, compute_bounds(max_depth), my_play, maximizing);
      game_.unplay();
      game_.unplay();
      game_.unplay();
      game_.unplay();
    }

    // TODO:
    // - overruffs?
    // - finesses?
  }

  tpn_table_[game_.game_state()] = bounds;
  return bounds;
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
