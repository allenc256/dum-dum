#include "game_model.h"
#include "random.h"
#include "solver.h"

#include <argparse/argparse.hpp>
#include <chrono>
#include <iostream>

struct Options {
  std::optional<Suit> trump_suit;
  std::optional<Seat> lead_seat;
  int                 seed;
  int                 num_hands;
  int                 deal_size;
  bool                compact_output;
};

void parse_arguments(int argc, char **argv, Options &options) {
  argparse::ArgumentParser program("dumdum");

  std::string trumps_opt;
  std::string lead_opt;

  program.add_argument("-t", "--trumps")
      .choices("S", "H", "D", "C", "NT", "random")
      .default_value("random")
      .store_into(trumps_opt)
      .metavar("S")
      .nargs(1)
      .help("trump suit");
  program.add_argument("-l", "--lead")
      .choices("W", "N", "E", "S", "random")
      .default_value("random")
      .store_into(lead_opt)
      .metavar("S")
      .nargs(1)
      .help("lead seat");
  program.add_argument("-s", "--seed")
      .default_value(123)
      .store_into(options.seed)
      .nargs(1)
      .metavar("N")
      .help("random number generator seed");
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

  if (trumps_opt == "random") {
    options.trump_suit = std::nullopt;
  } else {
    std::istringstream is(trumps_opt);
    options.trump_suit.emplace(NO_TRUMP);
    is >> *options.trump_suit;
  }

  if (lead_opt == "random") {
    options.lead_seat = std::nullopt;
  } else {
    std::istringstream is(lead_opt);
    options.lead_seat.emplace(WEST);
    is >> *options.lead_seat;
  }
}

struct Solve {
  Seat    lead_seat;
  Suit    trump_suit;
  Hands   hands;
  int     tricks;
  int64_t states_explored;
  int64_t states_memoized;
  int64_t elapsed_ms;
};

void print_solve_compact_headers() {
  std::cout << "lead_seat,";
  std::cout << "trump_suit,";
  std::cout << "hands,";
  std::cout << "tricks,";
  std::cout << "states_explored,";
  std::cout << "states_memoized,";
  std::cout << "elapsed_ms\n";
}

constexpr const char *TRUMP_STRS[] = {"C", "D", "H", "S", "NT"};

void print_solve_compact(const Solve &solve) {
  std::cout << solve.lead_seat << ',';
  std::cout << TRUMP_STRS[solve.trump_suit] << ',';
  std::cout << solve.hands << ',';
  std::cout << solve.tricks << ',';
  std::cout << solve.states_explored << ',';
  std::cout << solve.states_memoized << ',';
  std::cout << solve.elapsed_ms << '\n';
}

void print_solve(const Solve &solve) {
  std::cout << "lead_seat        " << solve.lead_seat << '\n';
  std::cout << "trump_suit       " << TRUMP_STRS[solve.trump_suit] << '\n';
  std::cout << "hands            " << solve.hands << '\n';
  std::cout << "tricks           " << solve.tricks << '\n';
  std::cout << "states_explored  " << solve.states_explored << '\n';
  std::cout << "states_memoized  " << solve.states_memoized << '\n';
  std::cout << "elapsed_ms       " << solve.elapsed_ms << '\n';
  std::cout << '\n';
}

void solve_random(const Options &options) {
  Random random(options.seed);

  if (options.compact_output) {
    print_solve_compact_headers();
  }

  for (int i = 0; i < options.num_hands; i++) {
    Game g = random.random_game(
        options.trump_suit, options.lead_seat, options.deal_size
    );
    Solver s(g);
    auto   begin = std::chrono::steady_clock::now();
    auto   r     = s.solve();
    auto   end   = std::chrono::steady_clock::now();
    auto   elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();

    Solve solve = {
        .lead_seat       = g.lead_seat(),
        .trump_suit      = g.trump_suit(),
        .hands           = g.hands(),
        .tricks          = r.tricks_taken_by_ns,
        .states_explored = s.states_explored(),
        .states_memoized = s.states_memoized(),
        .elapsed_ms      = elapsed_ms,
    };

    if (options.compact_output) {
      print_solve_compact(solve);
    } else {
      print_solve(solve);
    }
  }
}

int main(int argc, char **argv) {
  Options options;
  parse_arguments(argc, argv, options);
  solve_random(options);
  return 0;
}
