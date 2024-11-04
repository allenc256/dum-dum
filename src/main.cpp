#include "game_model.h"
#include "random.h"
#include "solver.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>

void benchmark(int num_games, int cards_per_hand) {
  std::printf(
      "%20s,%20s,%20s,%20s,%20s\n",
      "seed",
      "tricks_taken_by_ns",
      "states_explored",
      "states_memoized",
      "elapsed_ms"
  );

  for (int i = 0; i < num_games; i++) {
    Game   g = Random(i).random_game(cards_per_hand);
    Solver s(g);

    auto begin = std::chrono::steady_clock::now();
    auto r     = s.solve();
    auto end   = std::chrono::steady_clock::now();
    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();

    std::printf(
        "%20d,%20d,%20lld,%20lld,%20lld\n",
        i,
        r.tricks_taken_by_ns,
        s.states_explored(),
        s.states_memoized(),
        elapsed_ms
    );
  }
}

void print_deal(const Hands &hands) {
  for (Seat seat = FIRST_SEAT; seat <= LAST_SEAT; seat++) {
    if (seat != FIRST_SEAT) {
      std::cout << '/';
    }
    Cards hand = hands.hand(seat);
    for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
      if (suit != LAST_SUIT) {
        std::cout << '.';
      }
      Cards cards = hand.intersect(suit);
      for (auto it = cards.iter_highest(); it.valid();
           it      = cards.iter_lower(it)) {
        std::cout << it.card().rank();
      }
    }
  }
}

constexpr const char *TRUMP_STRS[] = {"C", "D", "H", "S", "NT"};

void create_training_data(
    int                 seed,
    std::optional<Suit> trump_suit,
    std::optional<Seat> lead_seat,
    int                 num_games,
    int                 cards_per_hand
) {
  Random random(seed);

  std::cout << "lead,trumps,hands,tricks\n";

  for (int i = 0; i < num_games; i++) {
    Game   g = random.random_game(trump_suit, lead_seat, cards_per_hand);
    Solver s(g);
    auto   r = s.solve();
    std::cout << g.next_seat() << ',';
    std::cout << TRUMP_STRS[g.trump_suit()] << ',';
    print_deal(g.hands());
    std::cout << ',' << r.tricks_taken_by_ns << '\n';
  }
}

void solve_seed(int seed, int cards_per_hand) {
  Game   g = Random(seed).random_game(cards_per_hand);
  Solver s(g);

  auto begin = std::chrono::steady_clock::now();
  auto r     = s.solve();
  auto end   = std::chrono::steady_clock::now();
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();

  std::cout << g << std::endl;
  std::cout << "best_tricks_by_ns:  " << r.tricks_taken_by_ns << std::endl
            << "states_explored:    " << s.states_explored() << std::endl
            << "states_memoized:    " << s.states_memoized() << std::endl
            << "elapsed_ms:         " << elapsed_ms << std::endl;

  auto &stats = s.tpn_table_stats();
  std::cout << std::setfill('0');
  for (int i = 0; i < 13; i++) {
    std::cout << "states_memoized_" << std::setw(2) << i << ": "
              << stats.states_by_depth[i] << std::endl;
  }
  std::cout << std::setfill(' ');

  while (!s.game().finished()) {
    s.game().play(r.best_play);
    r = s.solve();
  }
  std::cout << std::left;
  char buf[256];
  for (int i = 0; i < s.game().tricks_taken(); i++) {
    snprintf(buf, sizeof(buf), "trick_%d:", i);
    std::cout << std::setw(20) << buf << s.game().trick(i) << std::endl;
  }
}

ABSL_FLAG(std::optional<std::string>, trumps, std::nullopt, "trump suit");
ABSL_FLAG(std::optional<std::string>, lead, std::nullopt, "lead seat");
ABSL_FLAG(int, seed, 123, "random number generator seed");
ABSL_FLAG(int, num_hands, 10, "number of hands to generate");
ABSL_FLAG(int, deal_size, 8, "number of cards per hand in each deal");

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);

  auto trumps_opt = absl::GetFlag(FLAGS_trumps);
  auto lead_opt   = absl::GetFlag(FLAGS_lead);
  int  seed       = absl::GetFlag(FLAGS_seed);
  int  num_hands  = absl::GetFlag(FLAGS_num_hands);
  int  deal_size  = absl::GetFlag(FLAGS_deal_size);

  std::optional<Suit> trumps;
  if (trumps_opt.has_value()) {
    std::istringstream is(*trumps_opt);
    trumps.emplace(NO_TRUMP);
    is >> *trumps;
  }

  std::optional<Seat> lead;
  if (lead_opt.has_value()) {
    std::istringstream is(*lead_opt);
    lead.emplace(WEST);
    is >> *lead;
  }

  create_training_data(seed, trumps, lead, num_hands, deal_size);

  // solve_seed(123, 13);

  return 0;
}
