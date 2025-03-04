#include <format>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include <vector>

#include "card_model.h"
#include "random.h"

using ::testing::ElementsAreArray;
using ::testing::IsEmpty;

TEST(Suit, parse_suit) {
  EXPECT_EQ(parse_suit("S"), SPADES);
  EXPECT_EQ(parse_suit("♣"), CLUBS);
  EXPECT_EQ(parse_suit("NT"), NO_TRUMP);
  EXPECT_THROW(parse_suit(""), Parser::Error);
  EXPECT_THROW(parse_suit("N"), Parser::Error);
}

TEST(Suit, format) {
  EXPECT_EQ(std::format("{}", SPADES), "♠");
  EXPECT_EQ(std::format("{}", NO_TRUMP), "NT");
}

TEST(Rank, parse_rank) {
  EXPECT_EQ(parse_rank("2"), RANK_2);
  EXPECT_EQ(parse_rank("A"), ACE);
  EXPECT_THROW(parse_rank("X"), Parser::Error);
}

TEST(Rank, format) {
  EXPECT_EQ(std::format("{}", RANK_2), "2");
  EXPECT_EQ(std::format("{}", ACE), "A");
}

TEST(Card, sizeof) { EXPECT_EQ(sizeof(Card), 1); }

TEST(Card, parse) {
  EXPECT_EQ(Card("3C"), Card(RANK_3, CLUBS));
  EXPECT_EQ(Card("T♥"), Card(TEN, HEARTS));
  EXPECT_THROW(Card("T"), Parser::Error);
}

TEST(Card, format) {
  EXPECT_EQ(std::format("{}", Card(RANK_5, DIAMONDS)), "5♦");
}

TEST(Cards, sizeof) { EXPECT_EQ(sizeof(Cards), sizeof(uint64_t)); }

TEST(Cards, parse) {
  std::array<std::string_view, 3> test_cases = {
      "T..432.KQJ",
      "AKQJT98765432.AKQJT98765432.AKQJT98765432.AKQJT98765432",
      "..."
  };

  for (std::string_view s : test_cases) {
    SCOPED_TRACE(s);
    std::string formatted = std::format("{}", Cards(s));
    EXPECT_EQ(s, formatted);
  }
}

std::vector<Card> iterate_cards(std::string s, bool high_to_low) {
  Cards             cs = Cards(s);
  std::vector<Card> result;
  if (high_to_low) {
    for (Card c : cs.high_to_low()) {
      result.push_back(c);
    }
  } else {
    for (Card c : cs.low_to_high()) {
      result.push_back(c);
    }
  }
  return result;
}

TEST(Cards, iter_high_to_low) {
  EXPECT_THAT(iterate_cards("...", true), IsEmpty());
  EXPECT_THAT(
      iterate_cards("A...", true), ElementsAreArray({Card(ACE, SPADES)})
  );
  EXPECT_THAT(
      iterate_cards("A..432.KQ2", true),
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
  EXPECT_THAT(iterate_cards("...", false), IsEmpty());
  EXPECT_THAT(
      iterate_cards("A...", false), ElementsAreArray({Card(ACE, SPADES)})
  );
  EXPECT_THAT(
      iterate_cards("A..432.KQ2", false),
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
  EXPECT_THAT(Cards("T..432.KQJ").count(), 7);
  EXPECT_THAT(
      Cards("AKQJT98765432.AKQJT98765432.AKQJT98765432.AKQJT98765432").count(),
      52
  );
  EXPECT_THAT(Cards("...").count(), 0);
}

TEST(Cards, disjoint) {
  Cards c1 = Cards("T..432.KQJ");
  Cards c2 = Cards("T...");
  Cards c3 = Cards("J..65.A");
  EXPECT_FALSE(c1.disjoint(c2));
  EXPECT_TRUE(c1.disjoint(c3));
}

TEST(Cards, normalize) {
  EXPECT_EQ(Cards("...K").normalize(Cards("...A")), Cards("...A"));
  EXPECT_EQ(Cards("...Q").normalize(Cards("...A")), Cards("...K"));
  EXPECT_EQ(Cards("J8..T3.").normalize(Cards("KT9..74.")), Cards("QJ..T5."));
  Cards c = Cards("KJ76.A.543.2");
  EXPECT_EQ(c.normalize(c.complement()), Cards("AKQJ.A.AKQ.A"));
  EXPECT_EQ(c.normalize(Cards()), c);
}

TEST(Cards, normalize_wbr) {
  Cards wbr("AK.AKQJT.A.");
  EXPECT_EQ(wbr.normalize_wbr(Cards()), wbr);
  EXPECT_EQ(wbr.normalize_wbr(wbr), Cards());
  EXPECT_EQ(wbr.normalize_wbr(Cards("AQJ.AKT9..")), Cards("A.AK.A."));
}

TEST(Cards, prune_equivalent) {
  EXPECT_EQ(Cards("...AK").prune_equivalent(Cards()), Cards("...A"));
  EXPECT_EQ(
      Cards("AKT98543.AQT953..432").prune_equivalent(Cards()),
      Cards("AT5.AQT53..4")
  );
  EXPECT_EQ(
      Cards("...542").prune_equivalent(Cards({"3♣", "6♣"})), Cards("...5")
  );
  Cards c = Cards("KJ983.5..732");
  EXPECT_EQ(c.prune_equivalent(c.complement()), Cards("K.5..7"));
}

TEST(Cards, lowest_equivalent_none_removed) {
  Cards c("AK32.T8..");
  EXPECT_EQ(c.lowest_equivalent(Card("A♠"), Cards()), Card("K♠"));
  EXPECT_EQ(c.lowest_equivalent(Card("3♠"), Cards()), Card("2♠"));
  EXPECT_EQ(c.lowest_equivalent(Card("T♥"), Cards()), Card("T♥"));
  EXPECT_EQ(c.lowest_equivalent(Card("8♥"), Cards()), Card("8♥"));
}

TEST(Cards, lowest_equivalent_removed) {
  Cards c("AK32.T8..");
  EXPECT_EQ(c.lowest_equivalent(Card("T♥"), Cards({"9♥"})), Card("8♥"));
  EXPECT_EQ(c.lowest_equivalent(Card("T♥"), c.complement()), Card("8♥"));
  EXPECT_EQ(c.lowest_equivalent(Card("K♠"), c.complement()), Card("2♠"));
}

TEST(SuitNormalizer, empty) {
  SuitNormalizer sn;
  for (Rank r = RANK_2; r <= ACE; r++) {
    EXPECT_EQ(sn.normalize(r), r);
    EXPECT_EQ(sn.denormalize(r), r);
  }
}

class NaiveNormalizer {
public:
  NaiveNormalizer(Random &random)
      : random_(random),
        removed_{false},
        removed_count_(0) {}

  Rank remove_random() {
    assert(removed_count_ < 13);
    while (true) {
      Rank to_remove = random_.random_rank();
      if (removed_[to_remove]) {
        continue;
      }
      removed_[to_remove] = true;
      removed_count_++;
      return to_remove;
    }
  }

  Rank add_random() {
    assert(removed_count_ > 0);
    while (true) {
      Rank to_add = random_.random_rank();
      if (!removed_[to_add]) {
        continue;
      }
      removed_[to_add] = false;
      removed_count_--;
      return to_add;
    }
  }

  Rank normalize(Rank rank) const {
    assert(!removed_[rank]);
    int result = rank;
    for (int r = rank + 1; r < 13; r++) {
      if (removed_[r]) {
        result++;
      }
    }
    return (Rank)result;
  }

  bool is_removed(Rank rank) const { return removed_[rank]; }
  int  removed_count() const { return removed_count_; }

private:
  Random               random_;
  std::array<bool, 13> removed_;
  int                  removed_count_;
};

TEST(SuitNormalizer, add_remove_random) {
  Random          random(123);
  NaiveNormalizer n1(random);
  SuitNormalizer  n2;

  for (int i = 0; i < 500; i++) {
    float p      = (float)n1.removed_count() / 13.0;
    bool  remove = random.random_uniform() > p;

    if (remove) {
      n2.remove(n1.remove_random());
    } else {
      n2.add(n1.add_random());
    }

    for (Rank rank = RANK_2; rank <= ACE; rank++) {
      if (!n1.is_removed(rank)) {
        Rank nr = n1.normalize(rank);
        ASSERT_EQ(n2.normalize(rank), nr);
        ASSERT_EQ(n2.denormalize(nr), rank);
      }
    }
  }
}
