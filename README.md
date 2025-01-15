# dum-dum

A [double dummy solver](https://en.wikipedia.org/wiki/Computer_bridge#Comparison_to_other_strategy_games) for bridge. This solver is optimized with a substantial number of game tree search techniques, described further in the [Technical Details](#technical-details) section below.

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

### Solve Random Hands

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

Example output (see [Representation](#representation) below for output format explanation):

```
$ src/dumdum random --compact --seed=12345 --deal=13 --hands=10
trumps    seat      tricks    elapsed   hands     
H         W         8         103       AKJ75.432.KQ7.J8/Q984.J85.854.A97/T32.76.AJ63.K542/6.AKQT9.T92.QT63
D         W         7         56        AJT84.4.T8653.95/Q92.QT82.Q74.KQ7/75.AJ9765.J92.T8/K63.K3.AK.AJ6432
H         E         10        13        AJ9832.J84.A.A74/K.KT96532.KQT7.3/QT4.Q.853.QJT962/765.A7.J9642.K85
S         S         12        8         974.J973.T92.J98/KJ5.K852.65.AK75/T862.T6.QJ8.QT42/AQ3.AQ4.AK743.63
D         N         10        7         KQ3.972.T642.742/J976.A6.AQ973.A5/A52.JT4.K85.JT63/T84.KQ853.J.KQ98
C         E         1         96        654.KJ74.J.AKT64/JT87.T93.Q642.85/Q3.AQ85.AKT5.QJ3/AK92.62.9873.972
D         N         9         154       9.AJT43.T965.QJ4/J85.6.QJ873.T976/KQT432.987.42.A3/A76.KQ52.AK.K852
S         W         8         477       94.K8732.T76.J54/Q73.Q96.83.AK832/AT82.J.KQJ94.QT7/KJ65.AT54.A52.96
S         N         9         68        K985.Q7.A92.AJ64/2.T98653.K6.KQ75/63.KJ4.QT87543.3/AQJT74.A2.J.T982
S         N         8         514       72.K4.K985.AJ984/AQJ95.T876.73.73/4.A952.AQT64.652/KT863.QJ3.J2.KQT

total_elapsed_ms   1496
avg_elapsed_ms     149
```

### Solve Hands From a File

Use `dumdum file` to solve hands stored in a file.

```
Usage: file [--help] [--version] [--compact] file

Solve hands read from a file.

Positional arguments:
  file           file containing hands to solve [required]

Optional arguments:
  -h, --help     shows help message and exits 
  -v, --version  prints version information and exits 
  -c, --compact  compact output
```

Example input (format should be `<HANDS> <SUIT> <SEAT>`, see [Representation](#representation) for additional details):

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
H         S         6         22        A2.T6.J7.KJ73/3.872.QT4.A94/JT.J95.863.T8/Q985.Q3.A2.52
S         S         6         1         J832.63.Q.QJ4/T95.AQ.T2.987/A4.JT8754.J9./Q7.K9.K75.A62
C         E         3         3         A7.Q86.K8.KQ6/J94.T7.T7.T72/K862.42.Q542./Q.AJ953.96.A3
D         N         9         0         J54.A32.J7.A5/A97.654..9863/63.98.832.JT4/KQT82.J.AKQ9.
NT        W         4         0         KJ.AQ.A962.JT/3.T3.KQ3.Q975/AT984.986.4.8/Q52.752.85.A4
D         E         8         1         KT765.64.T.T7/J.95.J63.AQ83/942.QJ.A8.J95/Q8.AKT.92.K62
NT        N         4         3         K82.QJT.T.T93/97.K52.K3.A82/AQT43.87.A97./J.643.Q64.K65
S         W         4         3         Q6.AK2.K98.A3/T75.43.JT.Q97/A3.T86.Q75.64/842.Q.42.KJT2
H         W         4         0         T82.A.J753.95/J9.3.9642.632/4.Q98764.K.T8/KQ53.J5.A.AK4
NT        S         6         1         9532.J9.52.98/7.865.QT987.J/QT8.AK2.A3.Q6/AKJ.73.64.KT2

total_elapsed_ms   34
avg_elapsed_ms     3
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
