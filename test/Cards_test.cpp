#include "Cards.h"

#include <gtest/gtest.h>

#include <iostream>

template <class T> T test_read(std::string s) {
  std::istringstream is(s);
  T x;
  is >> x;
  return x;
}

template <class T> std::string test_write(T x) {
  std::ostringstream os;
  os << x;
  return os.str();
}

TEST(Rank, istream) {
  EXPECT_EQ(test_read<Rank>("2"), RANK_2);
  EXPECT_EQ(test_read<Rank>("T"), TEN);
  EXPECT_THROW(test_read<Rank>(""), ParseFailure);
  EXPECT_THROW(test_read<Rank>("XXX"), ParseFailure);
}

TEST(Rank, ostream) {
  EXPECT_EQ(test_write(RANK_2), "2");
  EXPECT_EQ(test_write(ACE), "A");
}

TEST(Suit, istream) {
  EXPECT_EQ(test_read<Suit>("C"), CLUBS);
  EXPECT_EQ(test_read<Suit>("S"), SPADES);
  EXPECT_EQ(test_read<Suit>("♣"), CLUBS);
  EXPECT_EQ(test_read<Suit>("♠"), SPADES);
  EXPECT_THROW(test_read<Suit>(""), ParseFailure);
  EXPECT_THROW(test_read<Suit>("XXX"), ParseFailure);
}

TEST(Suit, ostream) {
  EXPECT_EQ(test_write<Suit>(CLUBS), "♣");
  EXPECT_EQ(test_write<Suit>(SPADES), "♠");
}

TEST(Card, istream) {
  EXPECT_EQ(test_read<Card>("3C"), Card(RANK_3, CLUBS));
  EXPECT_EQ(test_read<Card>("T♥"), Card(TEN, HEARTS));
  EXPECT_THROW(test_read<Card>("T"), ParseFailure);
}

TEST(Card, ostream) { EXPECT_EQ(test_write(Card(RANK_5, DIAMONDS)), "5♦"); }

void test_read_write_cards(std::string s) {
  EXPECT_EQ(test_write(test_read<Cards>(s)), s);
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

  EXPECT_EQ(test_write(test_read<Cards>("♠ - ♥ - ♦ - ♣ -")), "♠ - ♥ - ♦ - ♣ -");
  EXPECT_THROW(test_read<Cards>("♠ TT ♥ - ♦ - ♣ -"), ParseFailure);
  EXPECT_THROW(test_read<Cards>("♠ T ♥ - ♦ 234"), ParseFailure);
}
