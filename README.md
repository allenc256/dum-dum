# dum-dum

A [double dummy solver](https://en.wikipedia.org/wiki/Computer_bridge#Comparison_to_other_strategy_games) for bridge. This solver is optimized with a number of game tree search techniques, described further in the [Technical Details](#technical-details) section below.

## Building

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

## Running

Use `dumdum random` to solve a collection of randomly generated hands:

```
Usage: random [--help] [--version] [--seed N] [--hands N] [--deal N] [--compact]

Solve randomly generated hands.

Optional arguments:
  -h, --help     shows help message and exits 
  -v, --version  prints version information and exits 
  -s, --seed N   initial random number generator seed [default: 1]
  -n, --hands N  number of hands to generate [default: 10]
  -d, --deal N   number of cards per hand in each deal [default: 8]
  -c, --compact  compact output
```

Example output:

```
$ dumdum random --compact --seed=10 --deal=13 --hands=10
trumps    seat      tricks    elapsed   hands     
♥         S         7         104       A42.T6.KJ7.KQJ73/63.8742.QT54.A94/JT.AJ95.9863.T86/KQ9875.KQ3.A2.52
♠         S         7         44        J8632.63.AQ.QJT4/T95.AQ.T632.K987/A4.JT8754.J98.53/KQ7.K92.K754.A62
♣         E         5         83        A73.KQ86.K8.KQ96/J94.T7.T7.T87542/K862.42.AQJ542.J/QT5.AJ953.963.A3
♦         N         8         53        J54.AQ32.JT7.A52/A97.654.6.KQ9863/63.K98.85432.JT4/KQT82.JT7.AKQ9.7
NT        W         7         136       KJ.AQ4.A9762.JT6/3.T3.KQJT3.KQ975/AT984.J986.4.832/Q7652.K752.85.A4
♦         E         9         23        KT765.643.KQT.T7/J3.975.J763.AQ83/942.QJ82.A85.J95/AQ8.AKT.942.K642
NT        N         7         31        K82.QJT.T85.T943/97.K52.K32.AQ872/AQT643.987.AJ97./J5.A643.Q64.KJ65
♠         W         7         84        Q6.AK2.K9863.A53/JT75.943.AJT.Q97/A93.T8765.Q75.64/K842.QJ.42.KJT82
♥         W         5         8         T872.A.J753.QJ95/J9.K32.T9642.632/4.QT98764.KQ8.T8/AKQ653.J5.A.AK74
NT        S         7         1         965432.J9.J52.98/7.865.QT987.AJ73/QT8.AKQT2.A3.Q64/AKJ.743.K64.KT52

total_elapsed_ms   567
avg_elapsed_ms     56
```

Use `dumdum file` to solve hands stored in a file.

Example input (format should be `<HANDS> <SUIT> <SEAT>`):

```
A2.T6.J7.KJ73/3.872.QT4.A94/JT.J95.863.T8/Q985.Q3.A2.52 ♥ S
J832.63.Q.QJ4/T95.AQ.T2.987/A4.JT8754.J9./Q7.K9.K75.A62 ♠ S
A7.Q86.K8.KQ6/J94.T7.T7.T72/K862.42.Q542./Q.AJ953.96.A3 ♣ E
J54.A32.J7.A5/A97.654..9863/63.98.832.JT4/KQT82.J.AKQ9. ♦ N
KJ.AQ.A962.JT/3.T3.KQ3.Q975/AT984.986.4.8/Q52.752.85.A4 NT W
KT765.64.T.T7/J.95.J63.AQ83/942.QJ.A8.J95/Q8.AKT.92.K62 ♦ E
K82.QJT.T.T93/97.K52.K3.A82/AQT43.87.A97./J.643.Q64.K65 NT N
Q6.AK2.K98.A3/T75.43.JT.Q97/A3.T86.Q75.64/842.Q.42.KJT2 ♠ W
T82.A.J753.95/J9.3.9642.632/4.Q98764.K.T8/KQ53.J5.A.AK4 ♥ W
9532.J9.52.98/7.865.QT987.J/QT8.AK2.A3.Q6/AKJ.73.64.KT2 NT S
```

Example output:

```
$ dumdum file hands.txt --compact
trumps    seat      tricks    elapsed   hands     
♥         S         6         9         A2.T6.J7.KJ73/3.872.QT4.A94/JT.J95.863.T8/Q985.Q3.A2.52
♠         S         6         1         J832.63.Q.QJ4/T95.AQ.T2.987/A4.JT8754.J9./Q7.K9.K75.A62
♣         E         3         4         A7.Q86.K8.KQ6/J94.T7.T7.T72/K862.42.Q542./Q.AJ953.96.A3
♦         N         9         1         J54.A32.J7.A5/A97.654..9863/63.98.832.JT4/KQT82.J.AKQ9.
NT        W         4         0         KJ.AQ.A962.JT/3.T3.KQ3.Q975/AT984.986.4.8/Q52.752.85.A4
♦         E         8         1         KT765.64.T.T7/J.95.J63.AQ83/942.QJ.A8.J95/Q8.AKT.92.K62
NT        N         4         4         K82.QJT.T.T93/97.K52.K3.A82/AQT43.87.A97./J.643.Q64.K65
♠         W         4         3         Q6.AK2.K98.A3/T75.43.JT.Q97/A3.T86.Q75.64/842.Q.42.KJT2
♥         W         4         0         T82.A.J753.95/J9.3.9642.632/4.Q98764.K.T8/KQ53.J5.A.AK4
NT        S         6         1         9532.J9.52.98/7.865.QT987.J/QT8.AK2.A3.Q6/AKJ.73.64.KT2

total_elapsed_ms   24
avg_elapsed_ms     2
```

### Representation

The format for a single hand is specified as `<SPADES>.<HEARTS>.<DIAMONDS>.<CLUBS>`. So, for example:

```
A42.T6.KJ7.KQJ73
||| || ||| |||||
||| || ||| +++++-- Club ranks
||| || +++-------- Diamond ranks
||| ++------------ Heart ranks
+++--------------- Spade ranks
```

The format of set of hands is specified as `<WEST>/<NORTH>/<EAST>/<SOUTH>`. So, for example:

```
A42.T6.KJ7.KQJ73/63.8742.QT54.A94/JT.AJ95.9863.T86/KQ9875.KQ3.A2.52
\              / \              / \              / \              /
 +------------+   +------------+   +------------+   +------------+
      West            North             East            South
```

Suits may be specified using either unicode symbols (e.g., `♥` or `H` for hearts). Seats are specified by letters (e.g., `S` for south).

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

* Because the minimax score for a bridge game (i.e., the number of tricks taken by a particular side) can only increase monotonically as search depth increases along a given search tree path, it is possible to prune search nodes based on "fast" or "sure" tricks. That is, by examining forced lines of play where we know we can take an additional N tricks, it may be possible to induce early alpha/beta search cutoffs.

    - To compute the number of fast tricks available, we implement a very fast to execute greedy search over forced lines of play.

    - The greedy search accounts for ruffs (either by partner or by either opponent), discards by partner or opponent, as well as transportation.

    - Specifically, to handle transportation, the greedy search will prioritize unblocking suits in hand before transporting to partner.
