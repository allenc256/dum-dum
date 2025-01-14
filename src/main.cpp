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

static int64_t solve_seed(int index, const Options &options) {
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

  std::ostream_iterator<char> out(std::cout);

  if (options.compact_output) {
    std::format_to(
        out,
        "{:<10}{:<10}{:<10}{:<10}{:<10}{}\n",
        seed,
        g.trump_suit(),
        g.next_seat(),
        r.tricks_taken_by_ns,
        elapsed_ms,
        g.hands()
    );
  } else {
    std::format_to(out, "seed               {}\n", seed);
    std::format_to(out, "hands              {}\n", g.hands());
    std::format_to(out, "trump_suit         {}\n", g.trump_suit());
    std::format_to(out, "next_seat          {}\n", g.next_seat());
    std::format_to(out, "best_tricks_by_ns  {}\n", r.tricks_taken_by_ns);
    std::format_to(out, "best_tricks_by_ew  {}\n", r.tricks_taken_by_ew);
    std::format_to(out, "nodes_explored     {}\n", stats.nodes_explored);
    std::format_to(out, "tpn_buckets        {}\n", tpn_stats.buckets);
    std::format_to(out, "tpn_entries        {}\n", tpn_stats.entries);
    std::format_to(out, "tpn_lookup_hits    {}\n", tpn_stats.lookup_hits);
    std::format_to(out, "tpn_lookup_misses  {}\n", tpn_stats.lookup_misses);
    std::format_to(out, "tpn_insert_hits    {}\n", tpn_stats.insert_hits);
    std::format_to(out, "tpn_insert_misses  {}\n", tpn_stats.insert_misses);
    std::format_to(out, "tpn_insert_reads   {}\n", tpn_stats.insert_reads);
    std::format_to(out, "elapsed_ms         {}\n", elapsed_ms);
    std::format_to(out, "\n");
  }

  return elapsed_ms;
}

int main(int argc, char **argv) {
  Options options;
  parse_arguments(argc, argv, options);

  std::cout << std::left;

  if (options.compact_output) {
    print_compact_output_headers();
  }

  int64_t total_ms = 0;
  for (int i = 0; i < options.num_hands; i++) {
    total_ms += solve_seed(i, options);
  }

  std::cout << '\n';
  std::cout << "total_elapsed_ms   " << total_ms << '\n';
  std::cout << "avg_elapsed_ms     " << (total_ms / options.num_hands) << '\n';

  return 0;
}
