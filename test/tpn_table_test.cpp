#include "random.h"
#include "tpn_table.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

TEST(Slice, empty) {
  Slice<int> slice;
  EXPECT_EQ(slice.size(), 0);
}

TEST(Slice, expand) {
  Slice<std::string> slice;
  slice.expand() = "one";
  slice.expand() = "two";
  EXPECT_THAT(slice, ElementsAre("one", "two"));
}

struct NaiveTpnBucket {
  struct Entry {
    Hands partition;
    int   lower_bound;
    int   upper_bound;
  };

  void
  tightest_bounds(const Hands &partition, int &lower_bound, int &upper_bound) {
    lower_bound = 0;
    upper_bound = 13;
    for (auto &entry : entries) {
      if (partition.contains_all(entry.partition)) {
        assert(entry.lower_bound <= upper_bound);
        assert(entry.upper_bound >= lower_bound);
        lower_bound = std::max(lower_bound, entry.lower_bound);
        upper_bound = std::min(upper_bound, entry.upper_bound);
      }
    }
  }

  bool lookup(const Hands &hands, int alpha, int beta) const {
    for (auto &entry : entries) {
      if (hands.contains_all(entry.partition) &&
          (entry.upper_bound <= alpha || entry.lower_bound >= beta)) {
        return true;
      }
    }
    return false;
  }

  void insert(const Hands &partition, int lower_bound, int upper_bound) {
    for (Entry &entry : entries) {
      if (entry.partition.contains_all(partition)) {
        entry.lower_bound = std::max(entry.lower_bound, lower_bound);
        entry.upper_bound = std::min(entry.upper_bound, upper_bound);
      }
    }
    entries.push_back({
        .partition   = partition,
        .lower_bound = lower_bound,
        .upper_bound = upper_bound,
    });
  }

  std::vector<Entry> entries;
};

Cards random_winners_by_rank(Random &random) {
  Cards winners_by_rank;
  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    Card card = {random.random_rank(), suit};
    winners_by_rank.add_all(Cards::higher_ranking(card));
  }
  return winners_by_rank;
}

Hands random_partition(Random &random, int cards_per_hand) {
  Hands hands = random.random_deal(cards_per_hand);
  return hands.make_partition(random_winners_by_rank(random));
}

TEST(TpnBucket, random) {
  Random random(123);

  TpnBucket      bucket1;
  NaiveTpnBucket bucket2;

  for (int i = 0; i < 500; i++) {
    Hands partition = random_partition(random, 3);

    int lower_bound, upper_bound;
    bucket2.tightest_bounds(partition, lower_bound, upper_bound);

    if (random.random_uniform() > 0.5) {
      lower_bound = std::min(lower_bound + 1, upper_bound);
    }
    if (random.random_uniform() > 0.5) {
      upper_bound = std::max(upper_bound - 1, lower_bound);
    }

    bucket1.insert(partition, lower_bound, upper_bound);
    bucket2.insert(partition, lower_bound, upper_bound);
  }

  bucket1.check_invariants();

  for (int i = 0; i < 1000; i++) {
    Hands hands = random.random_deal(4);
    for (int j = 0; j < 13; j++) {
      bool lookup1 = bucket1.lookup(hands, j, j + 1);
      bool lookup2 = bucket2.lookup(hands, j, j + 1);
      ASSERT_EQ(lookup1, lookup2);
    }
  }
}
