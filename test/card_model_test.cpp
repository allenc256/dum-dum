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
  EXPECT_EQ(from_string<Rank>("X"), RANK_UNKNOWN);
  EXPECT_THROW(from_string<Rank>(""), ParseFailure);
  EXPECT_THROW(from_string<Rank>("Y"), ParseFailure);
}

TEST(Rank, ostream) {
  EXPECT_EQ(to_string(RANK_2), "2");
  EXPECT_EQ(to_string(ACE), "A");
  EXPECT_EQ(to_string(RANK_UNKNOWN), "X");
}

TEST(Card, sizeof) { EXPECT_EQ(sizeof(Card), 2); }

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
  EXPECT_EQ(from_string<Card>("X♥"), Card(RANK_UNKNOWN, HEARTS));
  EXPECT_THROW(from_string<Card>("T"), ParseFailure);
}

TEST(Card, ostream) { EXPECT_EQ(to_string(Card(RANK_5, DIAMONDS)), "5♦"); }

void test_read_write_cards(std::string s) {
  EXPECT_EQ(to_string(from_string<Cards>(s)), s);
}

TEST(Cards, sizeof) { EXPECT_EQ(sizeof(Cards), sizeof(uint64_t)); }

TEST(Cards, iostream) {
  std::array<const char *, 3> test_cases = {
      "♠ T ♥ - ♦ 432 ♣ KQJ",
      "♠ AKQJT98765432 ♥ AKQJT98765432 ♦ AKQJT98765432 ♣ AKQJT98765432",
      "♠ - ♥ - ♦ - ♣ -"
  };

  for (const char *s : test_cases) {
    SCOPED_TRACE(s);
    test_read_write_cards(s);
  }

  EXPECT_THROW(from_string<Cards>("♠ TT ♥ - ♦ - ♣ -"), ParseFailure);
  EXPECT_THROW(from_string<Cards>("♠ T ♥ - ♦ 234"), ParseFailure);
}

std::vector<Card> iterate_cards(std::string s, bool high_to_low) {
  Cards             c = Cards(s);
  std::vector<Card> result;
  if (high_to_low) {
    for (auto it = c.iter_highest(); it.valid(); it = c.iter_lower(it)) {
      result.push_back(it.card());
    }
  } else {
    for (auto it = c.iter_lowest(); it.valid(); it = c.iter_higher(it)) {
      result.push_back(it.card());
    }
  }
  return result;
}

TEST(Cards, iter_high_to_low) {
  EXPECT_THAT(iterate_cards("♠ - ♥ - ♦ - ♣ -", true), IsEmpty());
  EXPECT_THAT(
      iterate_cards("♠ A ♥ - ♦ - ♣ -", true),
      ElementsAreArray({Card(ACE, SPADES)})
  );
  EXPECT_THAT(
      iterate_cards("♠ A ♥ - ♦ 432 ♣ KQ2", true),
      ElementsAreArray({
          Card(ACE, SPADES),
          Card(KING, CLUBS),
          Card(QUEEN, CLUBS),
          Card(RANK_4, DIAMONDS),
          Card(RANK_3, DIAMONDS),
          Card(RANK_2, DIAMONDS),
          Card(RANK_2, CLUBS),
      })
  );
}

TEST(Cards, iter_low_to_high) {
  EXPECT_THAT(iterate_cards("♠ - ♥ - ♦ - ♣ -", false), IsEmpty());
  EXPECT_THAT(
      iterate_cards("♠ A ♥ - ♦ - ♣ -", false),
      ElementsAreArray({Card(ACE, SPADES)})
  );
  EXPECT_THAT(
      iterate_cards("♠ A ♥ - ♦ 432 ♣ KQ2", false),
      ElementsAreArray({
          Card(RANK_2, CLUBS),
          Card(RANK_2, DIAMONDS),
          Card(RANK_3, DIAMONDS),
          Card(RANK_4, DIAMONDS),
          Card(QUEEN, CLUBS),
          Card(KING, CLUBS),
          Card(ACE, SPADES),
      })
  );
}
TEST(Cards, count) {
  EXPECT_THAT(Cards("♠ T ♥ - ♦ 432 ♣ KQJ").count(), 7);
  EXPECT_THAT(
      Cards("♠ AKQJT98765432 ♥ AKQJT98765432 ♦ AKQJT98765432 ♣ AKQJT98765432")
          .count(),
      52
  );
  EXPECT_THAT(Cards("♠ - ♥ - ♦ - ♣ -").count(), 0);
}

TEST(Cards, disjoint) {
  Cards c1 = Cards("♠ T ♥ - ♦ 432 ♣ KQJ");
  Cards c2 = Cards("♠ T ♥ - ♦ - ♣ -");
  Cards c3 = Cards("♠ J ♥ - ♦ 65 ♣ A");
  EXPECT_FALSE(c1.disjoint(c2));
  EXPECT_TRUE(c1.disjoint(c3));
}

TEST(Cards, collapse) {
  EXPECT_EQ(
      Cards("♠ - ♥ - ♦ - ♣ K").collapse(Cards("♠ - ♥ - ♦ - ♣ A")),
      Cards("♠ - ♥ - ♦ - ♣ A")
  );
  EXPECT_EQ(
      Cards("♠ - ♥ - ♦ - ♣ Q").collapse(Cards("♠ - ♥ - ♦ - ♣ A")),
      Cards("♠ - ♥ - ♦ - ♣ K")
  );
  EXPECT_EQ(
      Cards("♠ J8 ♥ - ♦ T3 ♣ -").collapse(Cards("♠ KT9 ♥ - ♦ 74 ♣ -")),
      Cards("♠ QJ ♥ - ♦ T5 ♣ -")
  );
  Cards c = Cards("♠ KJ76 ♥ A ♦ 543 ♣ 2");
  EXPECT_EQ(c.collapse(c.complement()), Cards("♠ AKQJ ♥ A ♦ AKQ ♣ A"));
  EXPECT_EQ(c.collapse(Cards()), c);
}

TEST(Cards, prune_equivalent) {
  EXPECT_EQ(
      Cards("♠ - ♥ - ♦ - ♣ AK").prune_equivalent(Cards()),
      Cards("♠ - ♥ - ♦ - ♣ A")
  );
  EXPECT_EQ(
      Cards("♠ AKT98543 ♥ AQT953 ♦ - ♣ 432").prune_equivalent(Cards()),
      Cards("♠ AT5      ♥ AQT53 ♦ - ♣ 4")
  );
  EXPECT_EQ(
      Cards("♠ - ♥ - ♦ - ♣ 542").prune_equivalent(Cards({"3♣", "6♣"})),
      Cards("♠ - ♥ - ♦ - ♣ 5")
  );
  Cards c = Cards("♠ KJ983 ♥ 5 ♦ - ♣ 732");
  EXPECT_EQ(c.prune_equivalent(c.complement()), Cards("♠ K ♥ 5 ♦ - ♣ 7"));
}

TEST(Cards, top_ranks) {
  EXPECT_EQ(Cards("♠ - ♥ - ♦ - ♣ -").top_ranks(CLUBS), 0);
  EXPECT_EQ(Cards("♠ - ♥ - ♦ - ♣ 432").top_ranks(CLUBS), 0);
  EXPECT_EQ(Cards("♠ - ♥ - ♦ - ♣ AKQJ").top_ranks(CLUBS), 4);
  EXPECT_EQ(Cards("♠ - ♥ - ♦ - ♣ AQ").top_ranks(CLUBS), 1);
  EXPECT_EQ(Cards("♠ AKQJ ♥ - ♦ - ♣ -").top_ranks(SPADES), 4);
  EXPECT_EQ(Cards("♠ AKJ ♥ - ♦ - ♣ -").top_ranks(SPADES), 2);
}
