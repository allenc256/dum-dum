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
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    EXPECT_EQ(trick.can_finish(seat), tc.poss_winners.contains(seat));
  }
}

INSTANTIATE_TEST_SUITE_P(
    AbsTrick,
    AbsTrickTest,
    testing::ValuesIn(ABS_TRICK_TEST_CASES),
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

struct ValidPlaysTestCase {
  AbsCards    west;
  AbsCards    north;
  AbsCards    east;
  AbsCards    south;
  Suit        trump_suit;
  Seat        lead_seat;
  const char *line;
  AbsCards    valid_plays;
};

void test_abs_game_valid_plays(ValidPlaysTestCase t) {
  AbsGame game = AbsGame(
      t.trump_suit,
      t.lead_seat,
      (AbsCards[4]){
          t.west,
          t.north,
          t.east,
          t.south,
      }
  );

  if (t.line) {
    std::istringstream is(t.line);
    while (!is.eof()) {
      if (game.trick_state() == AbsTrick::FINISHING) {
        Seat s;
        is >> s;
        game.finish_trick(s);
      } else {
        Card c;
        is >> c;
        game.play(c);
      }
      is >> std::ws;
    }
  }

  EXPECT_EQ(game.valid_plays(), t.valid_plays);
}

TEST(AbsGame, valid_plays_leads) {
  test_abs_game_valid_plays({
      .west        = AbsCards("♠ AK ♥ X ♦ - ♣ -"),
      .north       = AbsCards("♠ XX ♥ X ♦ - ♣ -"),
      .east        = AbsCards("♠ XX ♥ X ♦ - ♣ -"),
      .south       = AbsCards("♠ XX ♥ X ♦ - ♣ -"),
      .lead_seat   = WEST,
      .trump_suit  = NO_TRUMP,
      .valid_plays = AbsCards("♠ AK ♥ X ♦ - ♣ -"),
  });
}

TEST(AbsGame, valid_plays_follow_suit) {
  test_abs_game_valid_plays({
      .west        = AbsCards("♠ AK ♥ X ♦ - ♣ -"),
      .north       = AbsCards("♠ QX ♥ X ♦ - ♣ -"),
      .east        = AbsCards("♠ XX ♥ X ♦ - ♣ -"),
      .south       = AbsCards("♠ XX ♥ X ♦ - ♣ -"),
      .lead_seat   = WEST,
      .trump_suit  = NO_TRUMP,
      .line        = "A♠",
      .valid_plays = AbsCards("♠ QX ♥ - ♦ - ♣ -"),
  });
}

TEST(AbsGame, valid_plays_winner) {
  test_abs_game_valid_plays({
      .west        = AbsCards("♠ AK ♥ X  ♦ - ♣ -"),
      .north       = AbsCards("♠ -  ♥ XX ♦ J ♣ -"),
      .east        = AbsCards("♠ XX ♥ X  ♦ - ♣ -"),
      .south       = AbsCards("♠ XX ♥ X  ♦ - ♣ -"),
      .lead_seat   = WEST,
      .trump_suit  = NO_TRUMP,
      .line        = "A♠X♥X♠X♠ W",
      .valid_plays = AbsCards("♠ K ♥ X ♦ - ♣ -"),
  });
}
