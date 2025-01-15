#include <argparse/argparse.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <variant>

#include "game_model.h"
#include "random.h"
#include "solver.h"

struct FileOpts {
  std::string path;
  bool        compact_output;
};

struct RandomOpts {
  int  initial_seed;
  int  num_hands;
  int  deal_size;
  bool compact_output;
};

using Options = std::variant<FileOpts, RandomOpts>;

static Options parse_arguments(int argc, char **argv) {
  FileOpts   solve_opts;
  RandomOpts random_opts;

  argparse::ArgumentParser program("dumdum");

  argparse::ArgumentParser file("file");
  file.add_description("Solve hands read from a file.");
  file.add_argument("file")
      .help("file containing hands to solve")
      .store_into(solve_opts.path)
      .required();
  file.add_argument("-c", "--compact")
      .default_value(false)
      .implicit_value(true)
      .store_into(solve_opts.compact_output)
      .help("compact output");

  argparse::ArgumentParser random("random");
  random.add_description("Solve randomly generated hands.");
  random.add_argument("-s", "--seed")
      .default_value(1)
      .store_into(random_opts.initial_seed)
      .nargs(1)
      .metavar("N")
      .help("initial random number generator seed");
  random.add_argument("-n", "--hands")
      .default_value(10)
      .store_into(random_opts.num_hands)
      .nargs(1)
      .metavar("N")
      .help("number of hands to generate");
  random.add_argument("-d", "--deal")
      .default_value(8)
      .store_into(random_opts.deal_size)
      .nargs(1)
      .metavar("N")
      .help("number of cards per hand in each deal");
  random.add_argument("-c", "--compact")
      .default_value(false)
      .implicit_value(true)
      .store_into(random_opts.compact_output)
      .help("compact output");

  program.add_subparser(file);
  program.add_subparser(random);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  if (program.is_subcommand_used(file)) {
    return solve_opts;
  } else if (program.is_subcommand_used(random)) {
    return random_opts;
  } else {
    std::cerr << program;
    std::exit(1);
  }
}

static void print_compact_output_headers() {
  std::ostream_iterator<char> out(std::cout);
  std::format_to(
      out,
      "{:10}{:10}{:10}{:10}{:10}\n",
      "trumps",
      "seat",
      "tricks",
      "elapsed",
      "hands"
  );
}

static int64_t solve_game(Game &g, bool compact_output) {
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

  if (compact_output) {
    std::format_to(
        out,
        "{:<10}{:<10}{:<10}{:<10}{}\n",
        suit_to_ascii(g.trump_suit()),
        g.next_seat(),
        r.tricks_taken_by_ns,
        elapsed_ms,
        g.hands()
    );
  } else {
    std::string_view trumps = suit_to_ascii(g.trump_suit());
    std::format_to(out, "hands              {}\n", g.hands());
    std::format_to(out, "trump_suit         {}\n", trumps);
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

template <class Generator>
static void solve_games(Generator &game_generator, bool compact_output) {
  if (compact_output) {
    print_compact_output_headers();
  }

  int64_t total_ms  = 0;
  int     num_hands = 0;
  while (game_generator.has_next()) {
    Game game = game_generator.next();
    total_ms += solve_game(game, compact_output);
    num_hands++;
  }
  int64_t avg_ms = total_ms / num_hands;

  std::ostream_iterator<char> out(std::cout);
  std::format_to(out, "\n");
  std::format_to(out, "total_elapsed_ms   {}\n", total_ms);
  std::format_to(out, "avg_elapsed_ms     {}\n", avg_ms);
}

class RandomGenerator {
public:
  RandomGenerator(const RandomOpts &opts) : opts_(opts) {}

  bool has_next() const { return index_ < opts_.num_hands; }

  Game next() {
    int seed = opts_.initial_seed + index_;
    index_++;
    return Random(seed).random_game(opts_.deal_size);
  }

private:
  const RandomOpts &opts_;
  int               index_ = 0;
};

class FileGenerator {
public:
  FileGenerator(const std::string &path) : ifs_(path) {
    if (!ifs_) {
      throw std::runtime_error(std::format("failed to open file: {}", path));
    }
    std::getline(ifs_, next_);
  }

  bool has_next() const { return (bool)ifs_; }

  Game next() {
    assert(has_next());
    Parser parser(next_);
    Suit   trumps = parse_suit(parser);
    parser.skip_whitespace();
    Seat seat = parse_seat(parser);
    parser.skip_whitespace();
    Hands hands(parser);
    std::getline(ifs_, next_);
    return Game(trumps, seat, hands);
  }

private:
  std::ifstream ifs_;
  std::string   next_;
};

int main(int argc, char **argv) {
  Options options = parse_arguments(argc, argv);

  if (auto opts = std::get_if<FileOpts>(&options)) {
    FileGenerator generator(opts->path);
    solve_games(generator, opts->compact_output);
  } else if (auto opts = std::get_if<RandomOpts>(&options)) {
    RandomGenerator generator(*opts);
    solve_games(generator, opts->compact_output);
  } else {
    throw std::runtime_error("unrecognized opts"); // unreachable
  }

  return 0;
}
