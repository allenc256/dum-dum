#include "card_model.h"
#include <iostream>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAreArray;
using ::testing::IsEmpty;

template <class T> T from_string(std::string s) {
  std::istringstream is(s);
  T x;
  is >> x;
  return x;
}

template <class T> std::string to_string(T x) {
  std::ostringstream os;
  os << x;
  return os.str();
}

TEST(Rank, istream) {
  EXPECT_EQ(from_string<Rank>("2"), RANK_2);
  EXPECT_EQ(from_string<Rank>("T"), TEN);
  EXPECT_THROW(from_string<Rank>(""), ParseFailure);
  EXPECT_THROW(from_string<Rank>("XXX"), ParseFailure);
}

TEST(Rank, ostream) {
  EXPECT_EQ(to_string(RANK_2), "2");
  EXPECT_EQ(to_string(ACE), "A");
}

TEST(Suit, istream) {
  EXPECT_EQ(from_string<Suit>("C"), CLUBS);
  EXPECT_EQ(from_string<Suit>("S"), SPADES);
  EXPECT_EQ(from_string<Suit>("♣"), CLUBS);
  EXPECT_EQ(from_string<Suit>("♠"), SPADES);
  EXPECT_THROW(from_string<Suit>(""), ParseFailure);
  EXPECT_THROW(from_string<Suit>("XXX"), ParseFailure);
}

TEST(Suit, ostream) {
  EXPECT_EQ(to_string<Suit>(CLUBS), "♣");
  EXPECT_EQ(to_string<Suit>(SPADES), "♠");
}

TEST(Card, istream) {
  EXPECT_EQ(from_string<Card>("3C"), Card(RANK_3, CLUBS));
  EXPECT_EQ(from_string<Card>("T♥"), Card(TEN, HEARTS));
  EXPECT_THROW(from_string<Card>("T"), ParseFailure);
}

TEST(Card, ostream) { EXPECT_EQ(to_string(Card(RANK_5, DIAMONDS)), "5♦"); }

void test_read_write_cards(std::string s) {
  EXPECT_EQ(to_string(from_string<Cards>(s)), s);
}

TEST(Cards, iostream) {
  std::array<const char *, 3> test_cases = {
      "♠ T ♥ - ♦ 432 ♣ KQJ",
      "♠ AKQJT98765432 ♥ AKQJT98765432 ♦ AKQJT98765432 ♣ AKQJT98765432",
      "♠ - ♥ - ♦ - ♣ -"};

  for (const char *s : test_cases) {
    SCOPED_TRACE(s);
    test_read_write_cards(s);
  }

  EXPECT_THROW(from_string<Cards>("♠ TT ♥ - ♦ - ♣ -"), ParseFailure);
  EXPECT_THROW(from_string<Cards>("♠ T ♥ - ♦ 234"), ParseFailure);
}

std::vector<Card> iterate_cards(std::string s) {
  Cards c = from_string<Cards>(s);
  std::vector<Card> result;
  for (auto it = c.first(); it.valid(); it = c.next(it)) {
    result.push_back(it.card());
  }
  return result;
}

TEST(Cards, iteration) {
  EXPECT_THAT(iterate_cards("♠ - ♥ - ♦ - ♣ -"), IsEmpty());
  EXPECT_THAT(iterate_cards("♠ A ♥ - ♦ - ♣ -"),
              ElementsAreArray({Card(ACE, SPADES)}));
  EXPECT_THAT(iterate_cards("♠ A ♥ - ♦ 432 ♣ KQ2"), ElementsAreArray({
                                                        Card(ACE, SPADES),
                                                        Card(RANK_4, DIAMONDS),
                                                        Card(RANK_3, DIAMONDS),
                                                        Card(RANK_2, DIAMONDS),
                                                        Card(KING, CLUBS),
                                                        Card(QUEEN, CLUBS),
                                                        Card(RANK_2, CLUBS),
                                                    }));
}

TEST(Cards, count) {
  EXPECT_THAT(from_string<Cards>("♠ T ♥ - ♦ 432 ♣ KQJ").count(), 7);
  EXPECT_THAT(
      from_string<Cards>(
          "♠ AKQJT98765432 ♥ AKQJT98765432 ♦ AKQJT98765432 ♣ AKQJT98765432")
          .count(),
      52);
  EXPECT_THAT(from_string<Cards>("♠ - ♥ - ♦ - ♣ -").count(), 0);
}

TEST(Cards, disjoint) {
  Cards c1 = from_string<Cards>("♠ T ♥ - ♦ 432 ♣ KQJ");
  Cards c2 = from_string<Cards>("♠ T ♥ - ♦ - ♣ -");
  Cards c3 = from_string<Cards>("♠ J ♥ - ♦ 65 ♣ A");
  EXPECT_FALSE(c1.disjoint(c2));
  EXPECT_TRUE(c1.disjoint(c3));
}
