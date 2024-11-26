#include "game_model.h"
#include "random.h"
#include "solver.h"

#include <argparse/argparse.hpp>
#include <chrono>
#include <iostream>

struct Options {
  int  initial_seed;
  int  num_hands;
  int  deal_size;
  bool compact_output;
};

static void parse_arguments(int argc, char **argv, Options &options) {
  argparse::ArgumentParser program("dumdum");

  program.add_argument("-s", "--seed")
      .default_value(1)
      .store_into(options.initial_seed)
      .nargs(1)
      .metavar("N")
      .help("initial random number generator seed");
  program.add_argument("-h", "--hands")
      .default_value(10)
      .store_into(options.num_hands)
      .nargs(1)
      .metavar("N")
      .help("number of hands to generate");
  program.add_argument("-d", "--deal")
      .default_value(8)
      .store_into(options.deal_size)
      .nargs(1)
      .metavar("N")
      .help("number of cards per hand in each deal");
  program.add_argument("-c", "--compact")
      .default_value(false)
      .implicit_value(true)
      .store_into(options.compact_output)
      .help("compact output");

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }
}

static constexpr int COL_WIDTH = 10;

static void print_compact_output_headers() {
  std::cout << std::setw(COL_WIDTH) << "seed";
  std::cout << std::setw(COL_WIDTH) << "trumps";
  std::cout << std::setw(COL_WIDTH) << "seat";
  std::cout << std::setw(COL_WIDTH) << "tricks";
  std::cout << std::setw(COL_WIDTH) << "elapsed";
  std::cout << "hands\n";
}

static void solve_seed(int index, const Options &options) {
  int    seed = options.initial_seed + index;
  Game   g    = Random(seed).random_game(options.deal_size);
  Solver s(g);

  auto begin = std::chrono::steady_clock::now();
  auto r     = s.solve();
  auto end   = std::chrono::steady_clock::now();
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();

  auto  stats     = s.stats();
  auto &tpn_stats = stats.tpn_table_stats;

  if (options.compact_output) {
    std::cout << std::setw(COL_WIDTH) << seed;
    std::cout << std::setw(COL_WIDTH) << to_ascii(g.trump_suit());
    std::cout << std::setw(COL_WIDTH) << g.next_seat();
    std::cout << std::setw(COL_WIDTH) << r.tricks_taken_by_ns;
    std::cout << std::setw(COL_WIDTH) << elapsed_ms;
    std::cout << g.hands() << '\n';
  } else {
    std::cout << "seed               " << seed << '\n';
    std::cout << "hands              " << g.hands() << '\n';
    std::cout << "trump_suit         " << g.trump_suit() << '\n';
    std::cout << "next_seat          " << g.next_seat() << '\n';
    std::cout << "best_tricks_by_ns  " << r.tricks_taken_by_ns << '\n';
    std::cout << "best_tricks_by_ew  " << r.tricks_taken_by_ew << '\n';
    std::cout << "nodes_explored     " << stats.nodes_explored << '\n';
    std::cout << "tpn_buckets        " << tpn_stats.buckets << '\n';
    std::cout << "tpn_entries        " << tpn_stats.entries << '\n';
    std::cout << "tpn_lookup_hits    " << tpn_stats.lookup_hits << '\n';
    std::cout << "tpn_lookup_misses  " << tpn_stats.lookup_misses << '\n';
    std::cout << "tpn_lookup_reads   " << tpn_stats.lookup_reads << '\n';
    std::cout << "tpn_insert_hits    " << tpn_stats.insert_hits << '\n';
    std::cout << "tpn_insert_misses  " << tpn_stats.insert_misses << '\n';
    std::cout << "tpn_insert_reads   " << tpn_stats.insert_reads << '\n';
    std::cout << "elapsed_ms         " << elapsed_ms << '\n';
    std::cout << std::endl;
  }
}

int main(int argc, char **argv) {
  Options options;
  parse_arguments(argc, argv, options);

  std::cout << std::left;

  if (options.compact_output) {
    print_compact_output_headers();
  }

  for (int i = 0; i < options.num_hands; i++) {
    solve_seed(i, options);
  }

  return 0;
}
