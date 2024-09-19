#include "card_model.h"
#include "test_util.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

using ::testing::ElementsAreArray;
using ::testing::IsEmpty;

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

TEST(Card, sizeof) { EXPECT_TRUE(sizeof(Card) <= sizeof(int)); }

TEST(Suit, istream) {
  EXPECT_EQ(from_string<Suit>("C"), CLUBS);
  EXPECT_EQ(from_string<Suit>("S"), SPADES);
  EXPECT_EQ(from_string<Suit>("♣"), CLUBS);
  EXPECT_EQ(from_string<Suit>("♠"), SPADES);
  EXPECT_EQ(from_string<Suit>("NT"), NO_TRUMP);
  EXPECT_THROW(from_string<Suit>(""), ParseFailure);
  EXPECT_THROW(from_string<Suit>("XXX"), ParseFailure);
  EXPECT_THROW(from_string<Suit>("N"), ParseFailure);

  char bad_str1[] = {(char)0xE2, 0x00};
  char bad_str2[] = {(char)0xE2, (char)0x99, 0x00};
  EXPECT_THROW(from_string<Suit>(bad_str1), ParseFailure);
  EXPECT_THROW(from_string<Suit>(bad_str2), ParseFailure);
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

TEST(Cards, sizeof) { EXPECT_TRUE(sizeof(Cards) == sizeof(uint64_t)); }

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
  Cards c = Cards(s);
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
  EXPECT_THAT(Cards("♠ T ♥ - ♦ 432 ♣ KQJ").count(), 7);
  EXPECT_THAT(
      Cards("♠ AKQJT98765432 ♥ AKQJT98765432 ♦ AKQJT98765432 ♣ AKQJT98765432")
          .count(),
      52);
  EXPECT_THAT(Cards("♠ - ♥ - ♦ - ♣ -").count(), 0);
}

TEST(Cards, disjoint) {
  Cards c1 = Cards("♠ T ♥ - ♦ 432 ♣ KQJ");
  Cards c2 = Cards("♠ T ♥ - ♦ - ♣ -");
  Cards c3 = Cards("♠ J ♥ - ♦ 65 ♣ A");
  EXPECT_FALSE(c1.disjoint(c2));
  EXPECT_TRUE(c1.disjoint(c3));
}

TEST(Cards, collapse_rank) {
  EXPECT_EQ(Cards::collapse_rank(Card("AC"), Cards("♠ - ♥ - ♦ - ♣ J3")),
            Card("QC"));
  EXPECT_EQ(Cards::collapse_rank(Card("8C"), Cards("♠ - ♥ - ♦ - ♣ T3")),
            Card("7C"));
  EXPECT_EQ(
      Cards::collapse_rank(Card("8C"), Cards().with(Card("8C")).complement()),
      Card("2C"));
  EXPECT_EQ(Cards::collapse_rank(Card("8C"),
                                 Cards().complement().intersect_suit(SPADES)),
            Card("8C"));
}

TEST(Cards, collapse_ranks) {
  EXPECT_EQ(Cards("♠ - ♥ - ♦ - ♣ 3").collapse_ranks(Cards("♠ - ♥ - ♦ - ♣ 2")),
            Cards("♠ - ♥ - ♦ - ♣ 2"));
  EXPECT_EQ(Cards("♠ - ♥ - ♦ - ♣ 4").collapse_ranks(Cards("♠ - ♥ - ♦ - ♣ 2")),
            Cards("♠ - ♥ - ♦ - ♣ 3"));
  EXPECT_EQ(
      Cards("♠ AJ ♥ - ♦ T3 ♣ -").collapse_ranks(Cards("♠ KQ43 ♥ - ♦ 74 ♣ -")),
      Cards("♠ T9 ♥ - ♦ 83 ♣ -"));
  Cards c = Cards("♠ AJ76 ♥ A ♦ 543 ♣ 2");
  EXPECT_EQ(c.collapse_ranks(c.complement()), Cards("♠ 5432 ♥ 2 ♦ 432 ♣ 2"));
  EXPECT_EQ(c.collapse_ranks(Cards()), c);
}
