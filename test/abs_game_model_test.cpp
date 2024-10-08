#include "abs_game_model.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct AbsTrickCanFinishTestCase {
  const char    *name;
  Suit           trump_suit;
  Seat           lead_seat;
  const char    *cards[4];
  std::set<Seat> can_finish;
};

const AbsTrickCanFinishTestCase ABS_TRICK_CAN_FINISH_TEST_CASES[] = {
    {
        .name       = "no_trump",
        .trump_suit = NO_TRUMP,
        .lead_seat  = WEST,
        .cards      = {"2C", "3C", "4C", "5C"},
        .can_finish = {SOUTH},
    },
    {
        .name       = "no_trump_north_lead",
        .trump_suit = NO_TRUMP,
        .lead_seat  = NORTH,
        .cards      = {"2C", "3C", "4C", "5C"},
        .can_finish = {WEST},
    },
    {
        .name       = "no_trump_discard",
        .trump_suit = NO_TRUMP,
        .lead_seat  = WEST,
        .cards      = {"2C", "3C", "4C", "5H"},
        .can_finish = {EAST},
    },
    {
        .name       = "trump_no_ruff",
        .trump_suit = HEARTS,
        .lead_seat  = WEST,
        .cards      = {"2H", "3H", "4H", "5H"},
        .can_finish = {SOUTH},
    },
    {
        .name       = "trump_discard",
        .trump_suit = HEARTS,
        .lead_seat  = WEST,
        .cards      = {"2H", "3H", "AS", "AC"},
        .can_finish = {NORTH},
    },
    {
        .name       = "trump_ruff",
        .trump_suit = HEARTS,
        .lead_seat  = WEST,
        .cards      = {"2C", "3C", "4H", "5C"},
        .can_finish = {EAST},
    },
    {
        .name       = "trump_overruff",
        .trump_suit = HEARTS,
        .lead_seat  = WEST,
        .cards      = {"2C", "3C", "4H", "5H"},
        .can_finish = {SOUTH},
    },
    {
        .name       = "trump_ruff_and_sluff",
        .trump_suit = HEARTS,
        .lead_seat  = WEST,
        .cards      = {"2C", "3H", "4C", "5S"},
        .can_finish = {NORTH},
    },
    {
        .name       = "no_trump_unknown_loses",
        .trump_suit = NO_TRUMP,
        .lead_seat  = WEST,
        .cards      = {"XC", "XC", "QC", "TC"},
        .can_finish = {EAST},
    },
    {
        .name       = "no_trump_unknown_discard",
        .trump_suit = NO_TRUMP,
        .lead_seat  = WEST,
        .cards      = {"2C", "3C", "XH", "4C"},
        .can_finish = {SOUTH},
    },
    {
        .name       = "no_trump_unknown_wins",
        .trump_suit = NO_TRUMP,
        .lead_seat  = WEST,
        .cards      = {"XC", "XC", "5H", "XC"},
        .can_finish = {WEST, NORTH, SOUTH},
    },
    {
        .name       = "trump_unknown_ruff",
        .trump_suit = HEARTS,
        .lead_seat  = WEST,
        .cards      = {"2C", "XH", "XH", "4C"},
        .can_finish = {NORTH, EAST},
    },
    {
        .name       = "trump_unknown_overruff",
        .trump_suit = HEARTS,
        .lead_seat  = WEST,
        .cards      = {"2C", "XH", "3H", "4C"},
        .can_finish = {EAST},
    },
};

class AbsTrickCanFinishTest
    : public testing::TestWithParam<AbsTrickCanFinishTestCase> {};

TEST_P(AbsTrickCanFinishTest, test_case) {
  const auto &tc = GetParam();
  AbsTrick    trick;
  trick.play_starting_card(tc.lead_seat, tc.trump_suit, Card(tc.cards[0]));
  for (int i = 1; i < 4; i++) {
    trick.play_continuing_card(Card(tc.cards[i]));
  }
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    EXPECT_EQ(trick.can_finish(seat), tc.can_finish.contains(seat));
  }
}

INSTANTIATE_TEST_SUITE_P(
    AbsTrick,
    AbsTrickCanFinishTest,
    testing::ValuesIn(ABS_TRICK_CAN_FINISH_TEST_CASES),
    [](const auto &info) { return info.param.name; }
);

void test_abs_cards_istream(const char *str, AbsCards exp) {
  std::istringstream is(str);
  AbsCards           act;
  is >> act >> std::ws;
  EXPECT_TRUE(is.eof());
  EXPECT_EQ(act, exp);
}

TEST(AbsCards, istream_empty) {
  test_abs_cards_istream("S - H - D - C -", AbsCards());
}

TEST(AbsCards, istream_low) {
  test_abs_cards_istream(
      "S X H XX D XXX C XXXX", AbsCards(Cards(), (int8_t[4]){4, 3, 2, 1})
  );
}

TEST(AbsCards, istream_high) {
  test_abs_cards_istream(
      "S KQ H - D 54 C 32",
      AbsCards(Cards("S KQ H - D 54 C 32"), (int8_t[4]){0})
  );
}

TEST(AbsCards, istream_high_and_low) {
  test_abs_cards_istream(
      "S KQXXX H - D 54 C 32",
      AbsCards(Cards("S KQ H - D 54 C 32"), (int8_t[4]){0, 0, 0, 3})
  );
}

void test_abs_cards_iter(AbsCards cards) {
  Cards  act_high_cards   = Cards();
  int8_t act_low_cards[4] = {0};

  for (auto it = cards.iter(); it.valid(); it.next()) {
    Card c = it.card();
    if (c.is_rank_unknown()) {
      act_low_cards[c.suit()]++;
    } else {
      act_high_cards.add(c);
    }
  }

  EXPECT_EQ(cards.high_cards(), act_high_cards);
  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    EXPECT_EQ(cards.low_cards(suit) > 0, act_low_cards[suit] == 1);
  }
}

TEST(AbsCards, iter_empty) { test_abs_cards_iter(AbsCards("♠ - ♥ - ♦ - ♣ -")); }

TEST(AbsCards, iter_high_only) {
  test_abs_cards_iter(AbsCards("♠ J ♥ J ♦ - ♣ AKQ"));
}

TEST(AbsCards, iter_low_only) {
  test_abs_cards_iter(AbsCards("♠ X ♥ X ♦ XXX ♣ -"));
}

TEST(AbsCards, iter_high_and_low) {
  test_abs_cards_iter(AbsCards("♠ AQXXX ♥ XX ♦ - ♣ JX"));
}

struct CheckAbsGameArgs {
  Seat            next_seat;
  AbsCards        valid_plays;
  AbsTrick::State trick_state;
  int             tricks_taken;
  int             tricks_taken_by_ns;
};

void check_abs_game(const AbsGame &game, CheckAbsGameArgs a) {
  EXPECT_EQ(game.next_seat(), a.next_seat);
  if (game.trick_state() == AbsTrick::STARTING ||
      game.trick_state() == AbsTrick::STARTED) {
    EXPECT_EQ(game.valid_plays(), a.valid_plays);
  }
  EXPECT_EQ(game.trick_state(), a.trick_state);
  EXPECT_EQ(game.tricks_taken(), a.tricks_taken);
  EXPECT_EQ(game.tricks_taken_by_ns(), a.tricks_taken_by_ns);
}

TEST(AbsGame, play_unplay) {
  AbsGame game = AbsGame(
      NO_TRUMP,
      WEST,
      (AbsCards[4]){
          AbsCards("♠ TX ♥ X ♦ - ♣ -"),
          AbsCards("♠ QX ♥ A ♦ - ♣ -"),
          AbsCards("♠ JX ♥ - ♦ X ♣ -"),
          AbsCards("♠ -  ♥ X ♦ - ♣ T5"),
      }
  );

  CheckAbsGameArgs check_args[] = {
      {
          .next_seat          = WEST,
          .valid_plays        = AbsCards("♠ TX ♥ X ♦ - ♣ -"),
          .trick_state        = AbsTrick::STARTING,
          .tricks_taken       = 0,
          .tricks_taken_by_ns = 0,
      },
      {
          .next_seat          = NORTH,
          .valid_plays        = AbsCards("♠ QX ♥ - ♦ - ♣ -"),
          .trick_state        = AbsTrick::STARTED,
          .tricks_taken       = 0,
          .tricks_taken_by_ns = 0,
      },
      {
          .next_seat          = EAST,
          .valid_plays        = AbsCards("♠ JX ♥ - ♦ - ♣ -"),
          .trick_state        = AbsTrick::STARTED,
          .tricks_taken       = 0,
          .tricks_taken_by_ns = 0,
      },
      {
          .next_seat          = SOUTH,
          .valid_plays        = AbsCards("♠ - ♥ X ♦ - ♣ T5"),
          .trick_state        = AbsTrick::STARTED,
          .tricks_taken       = 0,
          .tricks_taken_by_ns = 0,
      },
      {
          .next_seat          = WEST,
          .trick_state        = AbsTrick::FINISHING,
          .tricks_taken       = 0,
          .tricks_taken_by_ns = 0,
      },
  };

  CheckAbsGameArgs north_finish_args = {
      .next_seat          = NORTH,
      .valid_plays        = AbsCards("♠ Q ♥ A ♦ - ♣ -"),
      .trick_state        = AbsTrick::STARTING,
      .tricks_taken       = 1,
      .tricks_taken_by_ns = 1,
  };
  CheckAbsGameArgs east_finish_args = {
      .next_seat          = EAST,
      .valid_plays        = AbsCards("♠ J ♥ - ♦ X ♣ -"),
      .trick_state        = AbsTrick::STARTING,
      .tricks_taken       = 1,
      .tricks_taken_by_ns = 0,
  };

  Card cards[4] = {Card("X♠"), Card("X♠"), Card("X♠"), Card("5♣")};

  for (int i = 0; i < 4; i++) {
    check_abs_game(game, check_args[i]);
    game.play(cards[i]);
    check_abs_game(game, check_args[i + 1]);
  }

  game.finish_trick(NORTH);
  check_abs_game(game, north_finish_args);
  game.unfinish_trick();
  check_abs_game(game, check_args[4]);
  game.finish_trick(EAST);
  check_abs_game(game, east_finish_args);
  game.unfinish_trick();

  for (int i = 3; i >= 0; i--) {
    check_abs_game(game, check_args[i + 1]);
    game.unplay();
    check_abs_game(game, check_args[i]);
  }
}
