#include "game_model.h"
#include "random.h"
#include "test_util.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

Trick make_trick(Suit trump_suit, Seat lead, std::vector<std::string> cards) {
  Trick t;
  t.play_start(trump_suit, lead, Card(cards[0]));
  for (int i = 1; i < cards.size(); i++) {
    t.play_continue(Card(cards[i]));
  }
  return t;
}

void test_trick(
    Suit                     trump_suit,
    Seat                     lead,
    std::vector<std::string> cards,
    Seat                     expected_winner
) {
  Trick t = make_trick(trump_suit, lead, cards);
  EXPECT_TRUE(t.started());
  EXPECT_TRUE(t.finished());
  EXPECT_EQ(t.winning_seat(), expected_winner);
}

TEST(Trick, ostream) {
  EXPECT_EQ(to_string(Trick()), "-");
  EXPECT_EQ(to_string(make_trick(NO_TRUMP, WEST, {"2C", "3C"})), "2♣3♣");
}

TEST(Trick, no_trump) {
  test_trick(NO_TRUMP, WEST, {"2C", "3C", "TC", "8C"}, EAST);
}

TEST(Trick, no_trump_discard) {
  test_trick(NO_TRUMP, WEST, {"JC", "3C", "TC", "AS"}, WEST);
}

TEST(Trick, trump_no_ruff) {
  test_trick(HEARTS, WEST, {"2C", "AC", "TC", "8C"}, NORTH);
}

TEST(Trick, trump_ruff) {
  test_trick(HEARTS, WEST, {"2C", "AC", "TC", "2H"}, SOUTH);
}

TEST(Trick, trump_overruff) {
  test_trick(HEARTS, WEST, {"2C", "2H", "3H", "TC"}, EAST);
}

TEST(Trick, trump_discard) {
  test_trick(HEARTS, WEST, {"2C", "2S", "3C", "TC"}, SOUTH);
}

TEST(Trick, trump_ruff_discard) {
  test_trick(HEARTS, WEST, {"2C", "2H", "3C", "TC"}, NORTH);
}

struct CheckTrickArgs {
  int  card_count;
  Seat winning_seat;
  bool is_null_play[4];
};

static void check_trick(const Trick &t, CheckTrickArgs args) {
  EXPECT_EQ(t.started(), args.card_count > 0);
  EXPECT_EQ(t.finished(), args.card_count == 4);
  EXPECT_EQ(t.card_count(), args.card_count);
  if (t.started()) {
    EXPECT_EQ(t.winning_seat(), args.winning_seat);
  }
  for (int i = 0; i < t.card_count(); i++) {
    EXPECT_EQ(!t.has_card(i), args.is_null_play[i]);
  }
}

TEST(Trick, unplay) {
  Trick t = make_trick(HEARTS, WEST, {"2C", "2H", "3C", "TC"});
  check_trick(
      t, {.card_count = 4, .winning_seat = NORTH, .is_null_play = {false}}
  );
  t.unplay();
  check_trick(
      t, {.card_count = 3, .winning_seat = NORTH, .is_null_play = {false}}
  );
  t.unplay();
  check_trick(
      t, {.card_count = 2, .winning_seat = NORTH, .is_null_play = {false}}
  );
  t.unplay();
  check_trick(
      t, {.card_count = 1, .winning_seat = WEST, .is_null_play = {false}}
  );
  t.unplay();
  check_trick(t, {.card_count = 0});
}

TEST(Trick, winner) {
  Trick t = Trick();
  t.play_start(HEARTS, WEST, Card("2C"));
  EXPECT_EQ(t.winning_index(), 0);
  t.play_continue(Card("3C"));
  EXPECT_EQ(t.winning_index(), 1);
  t.play_continue(Card("2H"));
  EXPECT_EQ(t.winning_index(), 2);
  t.play_continue(Card("AC"));
  EXPECT_EQ(t.winning_index(), 2);
  EXPECT_EQ(t.winning_seat(), EAST);
  EXPECT_EQ(t.winning_card(), Card("2H"));
  t.unplay();
  EXPECT_EQ(t.winning_index(), 2);
  t.unplay();
  EXPECT_EQ(t.winning_index(), 1);
  t.unplay();
  EXPECT_EQ(t.winning_index(), 0);
}

TEST(Trick, play_null) {
  Trick t;

  t.play_start(HEARTS, WEST, Card("2C"));
  check_trick(
      t, {.card_count = 1, .winning_seat = WEST, .is_null_play = {false}}
  );
  t.play_null();
  check_trick(
      t, {.card_count = 2, .winning_seat = WEST, .is_null_play = {false, true}}
  );
  t.play_continue(Card("3C"));
  check_trick(
      t, {.card_count = 3, .winning_seat = EAST, .is_null_play = {false, true}}
  );
  t.play_null();
  check_trick(
      t,
      {.card_count   = 4,
       .winning_seat = EAST,
       .is_null_play = {false, true, false, true}}
  );
  t.unplay();
  check_trick(
      t, {.card_count = 3, .winning_seat = EAST, .is_null_play = {false, true}}
  );
  t.unplay();
  check_trick(
      t, {.card_count = 2, .winning_seat = WEST, .is_null_play = {false, true}}
  );
  t.unplay();
  check_trick(
      t, {.card_count = 1, .winning_seat = WEST, .is_null_play = {false}}
  );
}

TEST(Trick, highest_card_no_trump_in_suit) {
  Trick trick = make_trick(NO_TRUMP, WEST, {"2C"});
  EXPECT_EQ(trick.highest_card(Cards({"3C", "TC", "AS"})), Card("TC"));
}

TEST(Trick, highest_card_no_trump_discard) {
  Trick trick = make_trick(NO_TRUMP, WEST, {"2C"});
  EXPECT_EQ(trick.highest_card(Cards({"3H", "TD", "AS"})), Card("AS"));
}

TEST(Trick, highest_card_trump_in_suit) {
  Trick trick = make_trick(HEARTS, WEST, {"2C"});
  EXPECT_EQ(trick.highest_card(Cards({"3C", "TC", "5H"})), Card("TC"));
}

TEST(Trick, highest_card_trump_ruff) {
  Trick trick = make_trick(HEARTS, WEST, {"2C"});
  EXPECT_EQ(trick.highest_card(Cards({"3S", "TS", "5H"})), Card("5H"));
}

TEST(Trick, is_higher_card_no_trump) {
  Trick trick = make_trick(NO_TRUMP, WEST, {"2C"});
  EXPECT_TRUE(trick.is_higher_card(Card("5C"), Card("4C")));
  EXPECT_FALSE(trick.is_higher_card(Card("4C"), Card("5C")));
  EXPECT_TRUE(trick.is_higher_card(Card("2C"), Card("4H")));
  EXPECT_FALSE(trick.is_higher_card(Card("4H"), Card("2C")));
}

TEST(Trick, is_higher_card_trump) {
  Trick trick = make_trick(HEARTS, WEST, {"2C"});
  EXPECT_TRUE(trick.is_higher_card(Card("5H"), Card("4H")));
  EXPECT_FALSE(trick.is_higher_card(Card("4H"), Card("5H")));
  EXPECT_TRUE(trick.is_higher_card(Card("2H"), Card("5C")));
  EXPECT_FALSE(trick.is_higher_card(Card("5C"), Card("2H")));
}

TEST(Game, play_unplay) {
  Hands hands = {
      Cards("S A2 H - D - C -"),
      Cards("S 93 H - D - C -"),
      Cards("S 5  H 2 D - C -"),
      Cards("S 6  H 3 D - C -")
  };
  Game g(HEARTS, WEST, hands);

  EXPECT_EQ(g.tricks_taken(), 0);
  EXPECT_EQ(g.next_seat(), WEST);
  EXPECT_EQ(g.tricks_taken_by_ns(), 0);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_EQ(g.hands(), hands);

  g.play(Card("2S"));
  g.play(Card("9S"));
  g.play(Card("5S"));
  g.play(Card("6S"));

  EXPECT_EQ(g.tricks_taken(), 1);
  EXPECT_EQ(g.next_seat(), NORTH);
  EXPECT_EQ(g.tricks_taken_by_ns(), 1);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(g.finished());

  g.play(Card("3S"));
  g.play(Card("2H"));
  g.play(Card("3H"));
  g.play(Card("AS"));

  EXPECT_EQ(g.tricks_taken(), 2);
  EXPECT_EQ(g.next_seat(), SOUTH);
  EXPECT_EQ(g.tricks_taken_by_ns(), 2);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_TRUE(g.finished());
  for (int i = 0; i < 4; i++) {
    EXPECT_TRUE(g.hand((Seat)i).empty());
  }

  g.unplay();

  EXPECT_EQ(g.tricks_taken(), 1);
  EXPECT_EQ(g.next_seat(), WEST);
  EXPECT_EQ(g.tricks_taken_by_ns(), 1);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(g.finished());

  g.unplay();
  g.unplay();
  g.unplay();

  EXPECT_EQ(g.tricks_taken(), 1);
  EXPECT_EQ(g.next_seat(), NORTH);
  EXPECT_EQ(g.tricks_taken_by_ns(), 1);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(g.finished());

  g.unplay();
  g.unplay();
  g.unplay();
  g.unplay();

  EXPECT_EQ(g.tricks_taken(), 0);
  EXPECT_EQ(g.next_seat(), WEST);
  EXPECT_EQ(g.tricks_taken_by_ns(), 0);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(g.finished());
  EXPECT_EQ(g.hands(), hands);

  EXPECT_THROW(g.unplay(), std::runtime_error);
}

TEST(Game, valid_plays_all) {
  Hands hands = {
      Cards("S A2 H - D - C -"),
      Cards("S 93 H - D - C -"),
      Cards("S 5  H 2 D - C -"),
      Cards("S 6  H 3 D - C -")
  };
  Game g(HEARTS, WEST, hands);

  EXPECT_EQ(g.valid_plays_all(), Cards("S A2 H - D - C -"));
  g.play(Card("2S"));
  EXPECT_EQ(g.valid_plays_all(), Cards("S 93 H - D - C -"));
  g.play(Card("9S"));
  EXPECT_EQ(g.valid_plays_all(), Cards("S 5  H - D - C -"));
  g.play(Card("5S"));
  EXPECT_EQ(g.valid_plays_all(), Cards("S 6  H - D - C -"));
  g.play(Card("6S"));
  EXPECT_EQ(g.valid_plays_all(), Cards("S 3  H - D - C -"));
  g.play(Card("3S"));
  EXPECT_EQ(g.valid_plays_all(), Cards("S -  H 2 D - C -"));
  g.play(Card("2H"));
  EXPECT_EQ(g.valid_plays_all(), Cards("S -  H 3 D - C -"));
  g.play(Card("3H"));
  EXPECT_EQ(g.valid_plays_all(), Cards("S A  H - D - C -"));
  g.play(Card("AS"));
  EXPECT_EQ(g.valid_plays_all(), Cards());
}

void test_play_unplay_dfs(Game &g) {
  bool  finished           = g.finished();
  Seat  next_seat          = g.next_seat();
  int   trick_count        = g.tricks_taken();
  int   tricks_taken_by_ew = g.tricks_taken_by_ew();
  int   tricks_taken_by_ns = g.tricks_taken_by_ns();
  Cards hands[4];
  for (int i = 0; i < 4; i++) {
    hands[i] = g.hand((Seat)i);
  }

  Cards p = g.valid_plays_all();
  for (Card c : p.high_to_low()) {
    g.play(c);
    test_play_unplay_dfs(g);
    g.unplay();

    EXPECT_EQ(g.finished(), finished);
    EXPECT_EQ(g.next_seat(), next_seat);
    EXPECT_EQ(g.tricks_taken(), trick_count);
    EXPECT_EQ(g.tricks_taken_by_ew(), tricks_taken_by_ew);
    EXPECT_EQ(g.tricks_taken_by_ns(), tricks_taken_by_ns);
    EXPECT_EQ(
        g.tricks_taken_by_ew() + g.tricks_taken_by_ns(), g.tricks_taken()
    );
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(g.hand(Seat(j)), hands[j]);
    }
  }
}

TEST(Game, play_unplay_random) {
  for (int seed = 0; seed < 500; seed++) {
    Game g = Random(seed).random_game(3);
    test_play_unplay_dfs(g);
  }
}

TEST(Game, play_null) {
  Hands hands = {
      Cards("S 2 H - D - C -"),
      Cards("S - H 2 D - C -"),
      Cards("S A H - D - C -"),
      Cards("S - H - D 2 C -"),
  };
  Game game(NO_TRUMP, WEST, hands);

  game.play(Card("2S"));
  game.play_null();
  game.play(Card("AS"));
  game.play_null();

  EXPECT_EQ(game.next_seat(), EAST);
  EXPECT_EQ(game.tricks_taken_by_ns(), 0);
  EXPECT_EQ(game.tricks_taken_by_ew(), 1);
  EXPECT_TRUE(game.finished());

  game.unplay();

  EXPECT_EQ(game.next_seat(), SOUTH);
  EXPECT_EQ(game.tricks_taken_by_ns(), 0);
  EXPECT_EQ(game.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(game.finished());

  game.unplay();
  game.unplay();
  game.unplay();

  EXPECT_EQ(game.next_seat(), WEST);
  EXPECT_EQ(game.tricks_taken_by_ns(), 0);
  EXPECT_EQ(game.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(game.finished());
}
