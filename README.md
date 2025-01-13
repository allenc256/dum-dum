# dum-dum

A [double dummy solver](https://en.wikipedia.org/wiki/Computer_bridge#Comparison_to_other_strategy_games) for bridge. This solver is optimized with a number of game tree search techniques, described further in the [Technical Details](#technical-details) section below.

## Building and Running

Building the solver requires:

* CMake (tested on 3.28.3)
* C++ compiler w/ C++20 support.
  - Clang 18.1.3 (Ubuntu 24.04.1 LTS)
  - Clang 15.0.0 (Apple Darwin)

To build using CMake:

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Where the CMake build type should be as appropriate (e.g., `Debug` for debug builds). The above builds two executable targets:

* `dumdum` - the solver executable (you may run `dumdum --help` for usage).
* `dumdum_test` - the solver test suite.

## Technical Details

We summarize the key aspects of the implementation below:

* Whenever possible, sets of cards are represented using 64-bit integer bitsets for efficiency. We utilize the C++20 standard bit-manipulation library `<bit>` heavily for calls such as `std::popcount()` and `std::countl_zero()` (these calls typically compile down to single instructions on modern ARM and x86 CPUs).

* The core search algorithm is a modified version of alpha-beta search named "partition search." This algorithm was described originally by Ginsberg [here](https://aaai.org/papers/034-aaai96-034-partition-search/), with another description given by Haglund & Hein [here](https://github.com/dds-bridge/dds/blob/develop/doc/Alg-dds_x.pdf). Partition search is particularly effective in solving bridge problems, as it seems to give order-of-magnitude speed improvements over vanilla alpha-beta search.

* The implementation uses a [transposition table](https://en.wikipedia.org/wiki/Transposition_table) to record previously searched states.

    - Only states where all played tricks are fully completed are recorded. Incomplete/partial tricks are not recorded.

    - The top level of a table is a hash table from 64-bit integer keys to buckets.

    - The 64-bit integer key records the shape of all player's hands as well as the player who is next to lead.

    - Each bucket contains a collection of trees encoding a set of partitions built up during partition search, as well as lower/upper bounds for the score for each partition.

    - Each tree is arranged hierarchically so that two invariants always hold (1) a parent node always generalizes a child node (i.e., the partition of states in the parent is a superset of the partition of states in the child) and (2) the bounds in a child node must be strictly tighter than the bounds in a parent node.

    - As new partitions and bounds are inserted during game tree search, care must be taken to maintain the above invariants. This may involve rearranging nodes, tightening bounds, and/or deleting nodes from each tree.

* Game states are normalized before insertion and/or lookup within the transposition table to increase the hit rate. Normalizing game states means shifting the ranks of all cards so that there are no "gaps" between successive ranks for all remaining cards.

* Play order during search is optimized to try to maximize the number of alpha/beta-cutoffs (i.e., pruned branches) encountered during search. Good play order leads to order-of-magnitude improvements in solve speed.

    - The most important search nodes to have good play order at are the nodes where a player must lead, as these nodes have the highest branching factor (there are no constraints on what a player can play).

    - At these nodes, we prioritize "forcing" plays where either the player or partner can take a sure trick. This causes alpha-beta bounds to update more quickly during search, thus leading to more search cutoffs.

    - Subsequently, we prioritize plays that draw trumps or establish long suits, where it makes sense.

    - For plays where we are not on lead, we rely on very simple heuristics (e.g., if you're third or last seat, try to win the trick if you can; otherwise play low).

* Because the minimax score for a bridge game (i.e., the number of tricks taken by a particular side) can only increase monotonically, it is possible to prune search nodes based on "fast" or "sure" tricks. That is, by examining forced lines of play where we know we can take an additional N tricks, it may be possible to induce early alpha/beta search cutoffs.

    - To compute the number of fast tricks available, we implement a very fast to execute greedy search over forced lines of play.

    - The greedy search accounts for ruffs (either by partner or by either opponent), discards by partner or opponent, as well as transportation.

    - Specifically, to handle transportation, the greedy search will prioritize unblocking suits in hand before transporting to partner.
