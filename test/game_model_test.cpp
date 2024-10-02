#include "game_model.h"
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
  bool skipped[4];
};

static void check_trick(const Trick &t, CheckTrickArgs args) {
  EXPECT_EQ(t.started(), args.card_count > 0);
  EXPECT_EQ(t.finished(), args.card_count == 4);
  EXPECT_EQ(t.card_count(), args.card_count);
  if (t.started()) {
    EXPECT_EQ(t.winning_seat(), args.winning_seat);
  }
  for (int i = 0; i < t.card_count(); i++) {
    EXPECT_EQ(t.skipped(i), args.skipped[i]);
  }
}

TEST(Trick, unplay) {
  Trick t = make_trick(HEARTS, WEST, {"2C", "2H", "3C", "TC"});
  check_trick(t, {.card_count = 4, .winning_seat = NORTH, .skipped = {false}});
  t.unplay();
  check_trick(t, {.card_count = 3, .winning_seat = NORTH, .skipped = {false}});
  t.unplay();
  check_trick(t, {.card_count = 2, .winning_seat = NORTH, .skipped = {false}});
  t.unplay();
  check_trick(t, {.card_count = 1, .winning_seat = WEST, .skipped = {false}});
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

TEST(Trick, skip) {
  Trick t;

  t.play_start(HEARTS, WEST, Card("2C"));
  check_trick(t, {.card_count = 1, .winning_seat = WEST, .skipped = {false}});
  t.skip_continue();
  check_trick(
      t, {.card_count = 2, .winning_seat = WEST, .skipped = {false, true}}
  );
  t.play_continue(Card("3C"));
  check_trick(
      t, {.card_count = 3, .winning_seat = EAST, .skipped = {false, true}}
  );
  t.skip_continue();
  check_trick(
      t,
      {.card_count   = 4,
       .winning_seat = EAST,
       .skipped      = {false, true, false, true}}
  );
  t.unplay();
  check_trick(
      t, {.card_count = 3, .winning_seat = EAST, .skipped = {false, true}}
  );
  t.unplay();
  check_trick(
      t, {.card_count = 2, .winning_seat = WEST, .skipped = {false, true}}
  );
  t.unplay();
  check_trick(t, {.card_count = 1, .winning_seat = WEST, .skipped = {false}});
}

TEST(Game, random_deal) {
  std::default_random_engine random(123);
  Game                       g = Game::random_deal(random, 13);

  for (int n = 0; n < 100; n++) {
    for (int i = 0; i < 4; i++) {
      for (int j = i + 1; j < 4; j++) {
        ASSERT_TRUE(g.hand((Seat)i).disjoint(g.hand((Seat)j)));
      }
    }

    int total = 0;
    for (int i = 0; i < 4; i++) {
      total += g.hand((Seat)i).count();
    }
    ASSERT_EQ(total, 52);
  }
}

TEST(Game, play_unplay) {
  Cards hands[4] = {
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
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(g.hand((Seat)i), hands[i]);
  }

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
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(g.hand((Seat)i), hands[i]);
  }

  EXPECT_THROW(g.unplay(), std::runtime_error);
}

TEST(Game, valid_plays) {
  Cards hands[4] = {
      Cards("S A2 H - D - C -"),
      Cards("S 93 H - D - C -"),
      Cards("S 5  H 2 D - C -"),
      Cards("S 6  H 3 D - C -")
  };
  Game g(HEARTS, WEST, hands);

  EXPECT_EQ(g.valid_plays(), Cards("S A2 H - D - C -"));
  g.play(Card("2S"));
  EXPECT_EQ(g.valid_plays(), Cards("S 93 H - D - C -"));
  g.play(Card("9S"));
  EXPECT_EQ(g.valid_plays(), Cards("S 5  H - D - C -"));
  g.play(Card("5S"));
  EXPECT_EQ(g.valid_plays(), Cards("S 6  H - D - C -"));
  g.play(Card("6S"));
  EXPECT_EQ(g.valid_plays(), Cards("S 3  H - D - C -"));
  g.play(Card("3S"));
  EXPECT_EQ(g.valid_plays(), Cards("S -  H 2 D - C -"));
  g.play(Card("2H"));
  EXPECT_EQ(g.valid_plays(), Cards("S -  H 3 D - C -"));
  g.play(Card("3H"));
  EXPECT_EQ(g.valid_plays(), Cards("S A  H - D - C -"));
  g.play(Card("AS"));
  EXPECT_EQ(g.valid_plays(), Cards());
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

  Cards p = g.valid_plays();
  for (auto i = p.iter_highest(); i.valid(); i = p.iter_lower(i)) {
    g.play(i.card());
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
  std::default_random_engine random;
  Game                       g = Game::random_deal(random, 3);
  for (int i = 0; i < 500; i++) {
    test_play_unplay_dfs(g);
  }
}

TEST(Game, skip) {
  Cards hands[4];
  hands[WEST]  = Cards("S 2 H - D - C -");
  hands[NORTH] = Cards("S - H 2 D - C -");
  hands[EAST]  = Cards("S A H - D - C -");
  hands[SOUTH] = Cards("S - H - D 2 C -");
  Game game(NO_TRUMP, WEST, hands);

  game.play(Card("2S"));
  game.skip();
  game.play(Card("AS"));
  game.skip();

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
