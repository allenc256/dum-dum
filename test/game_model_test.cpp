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

void test_trick(Suit trump_suit, Seat lead, std::vector<std::string> cards,
                Seat expected_winner) {
  Trick t = make_trick(trump_suit, lead, cards);
  EXPECT_TRUE(t.started());
  EXPECT_TRUE(t.finished());
  EXPECT_EQ(t.next_seat(), expected_winner);
}

TEST(Trick, ostream) {
  EXPECT_EQ(to_string(Trick()), "-");
  EXPECT_EQ(to_string(make_trick(NO_TRUMP, WEST, {"2C", "3C"})), "W:2♣ N:3♣");
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

TEST(Trick, unplay) {
  Trick t = make_trick(HEARTS, WEST, {"2C", "2H", "3C", "TC"});
  EXPECT_TRUE(t.finished());
  EXPECT_EQ(t.card_count(), 4);
  t.unplay();
  EXPECT_TRUE(t.started());
  EXPECT_FALSE(t.finished());
  EXPECT_EQ(t.card_count(), 3);
  t.unplay();
  t.unplay();
  t.unplay();
  EXPECT_FALSE(t.started());
  EXPECT_FALSE(t.finished());
  EXPECT_EQ(t.card_count(), 0);
}

TEST(Game, random_deal) {
  std::default_random_engine random(123);
  Game g = Game::random_deal(random, 13);

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
  Contract contract = Contract(1, HEARTS, NORTH);
  Cards hands[4] = {Cards("S A2 H - D - C -"), Cards("S 93 H - D - C -"),
                    Cards("S 5  H 2 D - C -"), Cards("S 6  H 3 D - C -")};
  Game g(contract, hands);

  EXPECT_EQ(g.trick_count(), 0);
  EXPECT_EQ(g.next_player(), WEST);
  EXPECT_EQ(g.tricks_taken_by_ns(), 0);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);

  g.play(Card("2S"));
  g.play(Card("9S"));
  g.play(Card("5S"));
  g.play(Card("6S"));

  EXPECT_EQ(g.trick_count(), 1);
  EXPECT_EQ(g.next_player(), NORTH);
  EXPECT_EQ(g.tricks_taken_by_ns(), 1);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(g.finished());

  g.play(Card("3S"));
  g.play(Card("2H"));
  g.play(Card("3H"));
  g.play(Card("AS"));

  EXPECT_EQ(g.trick_count(), 2);
  EXPECT_EQ(g.next_player(), SOUTH);
  EXPECT_EQ(g.tricks_taken_by_ns(), 2);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_TRUE(g.finished());

  g.unplay();

  EXPECT_EQ(g.trick_count(), 1);
  EXPECT_EQ(g.next_player(), WEST);
  EXPECT_EQ(g.tricks_taken_by_ns(), 1);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(g.finished());

  g.unplay();
  g.unplay();
  g.unplay();

  EXPECT_EQ(g.trick_count(), 1);
  EXPECT_EQ(g.next_player(), NORTH);
  EXPECT_EQ(g.tricks_taken_by_ns(), 1);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(g.finished());

  g.unplay();
  g.unplay();
  g.unplay();
  g.unplay();

  EXPECT_EQ(g.trick_count(), 0);
  EXPECT_EQ(g.next_player(), WEST);
  EXPECT_EQ(g.tricks_taken_by_ns(), 0);
  EXPECT_EQ(g.tricks_taken_by_ew(), 0);
  EXPECT_FALSE(g.finished());
}
