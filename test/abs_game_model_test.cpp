#include "abs_game_model.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct AbsTrickTestCase {
  const char    *name;
  Suit           trump_suit;
  Seat           lead_seat;
  const char    *cards[4];
  std::set<Seat> poss_winners;
};

const AbsTrickTestCase ABS_TRICK_TEST_CASES[] = {
    {
        .name         = "no_trump",
        .trump_suit   = NO_TRUMP,
        .lead_seat    = WEST,
        .cards        = {"2C", "3C", "4C", "5C"},
        .poss_winners = {SOUTH},
    },
    {
        .name         = "no_trump_north_lead",
        .trump_suit   = NO_TRUMP,
        .lead_seat    = NORTH,
        .cards        = {"2C", "3C", "4C", "5C"},
        .poss_winners = {WEST},
    },
    {
        .name         = "no_trump_discard",
        .trump_suit   = NO_TRUMP,
        .lead_seat    = WEST,
        .cards        = {"2C", "3C", "4C", "5H"},
        .poss_winners = {EAST},
    },
    {
        .name         = "trump_no_ruff",
        .trump_suit   = HEARTS,
        .lead_seat    = WEST,
        .cards        = {"2H", "3H", "4H", "5H"},
        .poss_winners = {SOUTH},
    },
    {
        .name         = "trump_discard",
        .trump_suit   = HEARTS,
        .lead_seat    = WEST,
        .cards        = {"2H", "3H", "AS", "AC"},
        .poss_winners = {NORTH},
    },
    {
        .name         = "trump_ruff",
        .trump_suit   = HEARTS,
        .lead_seat    = WEST,
        .cards        = {"2C", "3C", "4H", "5C"},
        .poss_winners = {EAST},
    },
    {
        .name         = "trump_overruff",
        .trump_suit   = HEARTS,
        .lead_seat    = WEST,
        .cards        = {"2C", "3C", "4H", "5H"},
        .poss_winners = {SOUTH},
    },
    {
        .name         = "trump_ruff_and_sluff",
        .trump_suit   = HEARTS,
        .lead_seat    = WEST,
        .cards        = {"2C", "3H", "4C", "5S"},
        .poss_winners = {NORTH},
    },
    {
        .name         = "no_trump_unknown_loses",
        .trump_suit   = NO_TRUMP,
        .lead_seat    = WEST,
        .cards        = {"XC", "XC", "QC", "TC"},
        .poss_winners = {EAST},
    },
    {
        .name         = "no_trump_unknown_discard",
        .trump_suit   = NO_TRUMP,
        .lead_seat    = WEST,
        .cards        = {"2C", "3C", "XH", "4C"},
        .poss_winners = {SOUTH},
    },
    {
        .name         = "no_trump_unknown_wins",
        .trump_suit   = NO_TRUMP,
        .lead_seat    = WEST,
        .cards        = {"XC", "XC", "5H", "XC"},
        .poss_winners = {WEST, NORTH, SOUTH},
    },
    {
        .name         = "trump_unknown_ruff",
        .trump_suit   = HEARTS,
        .lead_seat    = WEST,
        .cards        = {"2C", "XH", "XH", "4C"},
        .poss_winners = {NORTH, EAST},
    },
    {
        .name         = "trump_unknown_overruff",
        .trump_suit   = HEARTS,
        .lead_seat    = WEST,
        .cards        = {"2C", "XH", "3H", "4C"},
        .poss_winners = {EAST},
    },
};

class AbsTrickTest : public testing::TestWithParam<AbsTrickTestCase> {};

TEST_P(AbsTrickTest, test_case) {
  const auto &tc = GetParam();
  AbsTrick    trick;
  trick.play_starting_card(tc.lead_seat, tc.trump_suit, Card(tc.cards[0]));
  for (int i = 1; i < 4; i++) {
    trick.play_continuing_card(Card(tc.cards[i]));
  }
  EXPECT_TRUE(trick.finished());
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    EXPECT_EQ(trick.is_poss_winning(seat), tc.poss_winners.contains(seat));
  }
}

INSTANTIATE_TEST_SUITE_P(
    AbsTrick,
    AbsTrickTest,
    testing::ValuesIn(ABS_TRICK_TEST_CASES),
    [](const auto &info) { return info.param.name; }
);

void test_abs_cards_iter(Cards high_cards, int8_t low_cards[4]) {
  Cards  act_high_cards   = Cards();
  int8_t act_low_cards[4] = {0};

  AbsCards cards(high_cards, low_cards);

  for (auto it = cards.iter(); it.valid(); it.next()) {
    Card c = it.card();
    if (c.is_rank_unknown()) {
      act_low_cards[c.suit()]++;
    } else {
      act_high_cards.add(c);
    }
  }

  EXPECT_EQ(high_cards, act_high_cards);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(low_cards[i] > 0, act_low_cards[i] == 1);
  }
}

TEST(AbsCards, iter_empty) {
  test_abs_cards_iter(Cards(), (int8_t[4]){0, 0, 0, 0});
}

TEST(AbsCards, iter_high_only) {
  test_abs_cards_iter(Cards({"2C", "3C", "4C", "5S"}), (int8_t[4]){0, 0, 0, 0});
}

TEST(AbsCards, iter_low_only) {
  test_abs_cards_iter(Cards(), (int8_t[4]){0, 1, 2, 3});
}

TEST(AbsCards, iter_high_and_low) {
  test_abs_cards_iter(Cards({"2C", "3C", "4C", "5S"}), (int8_t[4]){0, 1, 2, 3});
}
