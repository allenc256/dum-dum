#include "game_solver.h"

#include <picosha2.h>

struct State {
  State(const Game &g, int alpha, int beta, bool should_normalize) {
    const Trick &t      = g.current_trick();
    trump_suit_         = g.contract().suit();
    trick_card_count_   = t.card_count();
    trick_lead_seat_    = t.started() ? t.lead_seat() : g.next_seat();
    tricks_taken_by_ns_ = g.tricks_taken_by_ns();
    tricks_taken_by_ew_ = g.tricks_taken_by_ew();
    alpha_              = alpha;
    beta_               = beta;

    for (int i = 0; i < 4; i++) {
      hands_[i] = g.hand((Seat)i);
    }
    for (int i = 0; i < trick_card_count_; i++) {
      trick_cards_[i] = t.card(i);
    }

    if (should_normalize) {
      *this = normalize_all();
    }
  }

  State normalize_all() const {
    State s = normalize_ranks().normalize_seats().normalize_suits();

    s.alpha_ -= tricks_taken_by_ns_;
    s.beta_ -= tricks_taken_by_ns_;
    s.tricks_taken_by_ns_ = 0;

    return s;
  }

  State normalize_ranks() const {
    Cards to_collapse;
    for (int i = 0; i < 4; i++) {
      to_collapse.add_all(hands_[i]);
    }
    for (int i = 0; i < trick_card_count_; i++) {
      to_collapse.add(trick_cards_[i]);
    }
    to_collapse = to_collapse.complement();

    State s = *this;
    for (int i = 0; i < 4; i++) {
      s.hands_[i] = hands_[i].collapse_ranks(to_collapse);
    }
    for (int i = 0; i < trick_card_count_; i++) {
      s.trick_cards_[i] = Cards::collapse_rank(trick_cards_[i], to_collapse);
    }
    return s;
  }

  State normalize_seats() const {
    State s = *this;
    if (trick_lead_seat_ == EAST || trick_lead_seat_ == SOUTH) {
      for (int i = 0; i < 4; i++) {
        s.hands_[i] = hands_[right_seat((Seat)i, 2)];
      }
      s.trick_lead_seat_ = right_seat(trick_lead_seat_, 2);
    }
    return s;
  }

  State normalize_suits() const {
    uint64_t suit_rank_bits[4];
    Suit     suit_map[4];

    for (int suit_i = 0; suit_i < 4; suit_i++) {
      uint64_t rank_bits = 0;
      for (int seat_i = 0; seat_i < 4; seat_i++) {
        rank_bits |= hands_[seat_i].rank_bits((Suit)suit_i) << (seat_i * 13);
      }
      suit_rank_bits[suit_i] = rank_bits;
    }

    if (trump_suit_ != NO_TRUMP) {
      suit_rank_bits[trump_suit_] = ~0;
    }

    for (int i = 0; i < 4; i++) {
      suit_map[i] = (Suit)i;
    }
    for (int i = 0; i < 4; i++) {
      if ((Suit)i != trump_suit_) {
        int best = i;
        for (int j = i + 1; j < 4; j++) {
          if ((Suit)j != trump_suit_ &&
              suit_rank_bits[j] > suit_rank_bits[best]) {
            best = j;
          }
        }
        if (i != best) {
          std::swap(suit_map[i], suit_map[best]);
          std::swap(suit_rank_bits[i], suit_rank_bits[best]);
        }
      }
    }

    State s = *this;
    for (int i = 0; i < 4; i++) {
      s.hands_[i] = hands_[i].permute_suits(suit_map);
    }
    for (int i = 0; i < trick_card_count_; i++) {
      const Card &c     = trick_cards_[i];
      s.trick_cards_[i] = Card(c.rank(), suit_map[c.suit()]);
    }

    return s;
  }

  Suit  trump_suit_;
  Cards hands_[4];
  Card  trick_cards_[4];
  int   trick_card_count_;
  Seat  trick_lead_seat_;
  int   tricks_taken_by_ns_;
  int   tricks_taken_by_ew_;
  int   alpha_;
  int   beta_;
};

void Solver::init_ttkey(
    TTKey &k, const Game &g, int alpha, int beta, bool normalize
) {
  State s(g, alpha, beta, normalize);

  for (int i = 0; i < 4; i++) {
    k.hands_[i]       = s.hands_[i].bits();
    Card c            = s.trick_cards_[i];
    k.trick_cards_[i] = (uint8_t)((c.rank() << 4) | c.suit());
  }

  assert(s.alpha_ >= INT8_MIN && s.alpha_ <= INT8_MAX);
  assert(s.beta_ >= INT8_MIN && s.beta_ <= INT8_MAX);
  assert(s.trick_card_count_ >= 0 && s.trick_card_count_ < 4);

  k.alpha_            = (int8_t)s.alpha_;
  k.beta_             = (int8_t)s.beta_;
  k.trick_card_count_ = (uint8_t)s.trick_card_count_;
  k.trick_lead_seat_  = s.trick_lead_seat_;
}

static void sha256_hash(const Solver::TTKey &k, uint8_t digest[32]) {
  uint8_t *in_begin = (uint8_t *)&k;
  uint8_t *in_end   = in_begin + sizeof(Solver::TTKey);
  picosha2::hash256(in_begin, in_end, digest, digest + 32);
}

class Solver::Tracer {
public:
  Tracer(std::ostream &os, int trace_depth)
      : os_(os),
        trace_depth_(trace_depth) {}
  ~Tracer() {}

  void trace_game(const Game &game) { os_ << game << std::endl; }

  void trace_play(Card card) { line_.push_back(card); }
  void trace_unplay() { line_.pop_back(); }

  void trace_terminal(const Game &game, int alpha, int beta) {
    trace_line(
        game, nullptr, alpha, beta, "terminal", game.tricks_taken_by_ns()
    );
  }

  void trace_table_lookup(
      const Game  &game,
      const TTKey &ttkey,
      int          alpha,
      int          beta,
      int          best_tricks_by_ns
  ) {
    trace_line(game, &ttkey, alpha, beta, "lookup", best_tricks_by_ns);
  }

  void trace_search_start(
      const Game &game, const TTKey &ttkey, int alpha, int beta
  ) {
    trace_line(game, &ttkey, alpha, beta, "start", -1);
  }

  void trace_search_end(
      const Game  &game,
      const TTKey &ttkey,
      int          alpha,
      int          beta,
      bool         alpha_beta_pruned,
      int          best_tricks_by_ns
  ) {
    trace_line(
        game,
        &ttkey,
        alpha,
        beta,
        alpha_beta_pruned ? "end_pruned" : "end",
        best_tricks_by_ns
    );
  }

private:
  void trace_line(
      const Game  &game,
      const TTKey *ttkey,
      int          alpha,
      int          beta,
      const char  *tag,
      int          best_tricks_by_ns
  ) {
    int depth = (int)line_.size();
    if (depth > trace_depth_) {
      return;
    }
    bool maximizing = game.next_seat() == NORTH || game.next_seat() == SOUTH;
    os_ << std::setw(2) << line_.size();
    os_ << " " << (maximizing ? "max" : "min");
    os_ << " " << std::setw(2) << alpha;
    os_ << " " << std::setw(2) << beta;
    if (best_tricks_by_ns >= 0) {
      os_ << " " << std::setw(2) << best_tricks_by_ns;
    } else {
      os_ << "   ";
    }
    os_ << " " << std::setw(10) << tag;

    os_ << " ";
    constexpr int sha256_limit = 6;
    if (ttkey) {
      uint8_t sha256[32];
      sha256_hash(*ttkey, sha256);
      char old_fill = os_.fill();
      os_ << std::hex << std::setfill('0');
      for (int i = 0; i < sha256_limit; i++) {
        os_ << std::setw(2) << (int)sha256[i];
      }
      os_ << std::dec << std::setfill(old_fill);
    } else {
      for (int i = 0; i < sha256_limit; i++) {
        os_ << "  ";
      }
    }

    os_ << " ";
    for (auto c : line_) {
      os_ << c;
    }
    os_ << std::endl;
  }

  std::ostream     &os_;
  std::vector<Card> line_;
  int               trace_depth_;
};

Solver::Solver(Game g) : game_(g), states_explored_(0) {
  enable_all_optimizations(true);
}

Solver::~Solver() {}

void Solver::enable_tracing(std::ostream &os) { enable_tracing(os, 52); }

void Solver::enable_tracing(std::ostream &os, int trace_depth) {
  tracer_ = std::make_unique<Tracer>(os, trace_depth);
}

Solver::Result Solver::solve() {
  states_explored_ = 0;
  Card best_play;
  if (tracer_) {
    tracer_->trace_game(game_);
  }
  int tricks_taken_by_ns = solve_internal(0, game_.tricks_max(), &best_play);
  return Solver::Result(tricks_taken_by_ns, best_play, states_explored_);
}

int Solver::solve_internal(int alpha, int beta, Card *best_play) {
  if (game_.finished()) {
    if (tracer_) {
      tracer_->trace_terminal(game_, alpha, beta);
    }
    return game_.tricks_taken_by_ns();
  }

  TTKey ttkey;
  if (!best_play && transposition_table_enabled_) {
    init_ttkey(ttkey, game_, alpha, beta, state_normalization_);
    auto it = transposition_table_.find(ttkey);
    if (it != transposition_table_.end()) {
      int best_tricks_by_ns = game_.tricks_taken_by_ns() + it->second;
      if (tracer_) {
        tracer_->trace_table_lookup(
            game_, ttkey, alpha, beta, best_tricks_by_ns
        );
      }
      return best_tricks_by_ns;
    }
  }

  states_explored_++;

  bool maximizing;
  int  best_tricks_by_ns;
  if (game_.next_seat() == NORTH || game_.next_seat() == SOUTH) {
    maximizing        = true;
    best_tricks_by_ns = -1;
  } else {
    maximizing        = false;
    best_tricks_by_ns = game_.tricks_max() + 1;
  }

  if (tracer_) {
    tracer_->trace_search_start(game_, ttkey, alpha, beta);
  }

  bool prune = solve_internal_search_plays(
      maximizing, alpha, beta, best_tricks_by_ns, best_play
  );

  if (tracer_) {
    tracer_->trace_search_end(
        game_, ttkey, alpha, beta, prune, best_tricks_by_ns
    );
  }

  if (!best_play && transposition_table_enabled_) {
    int tricks_takable_by_ns = best_tricks_by_ns - game_.tricks_taken_by_ns();
    assert(tricks_takable_by_ns >= 0);
    transposition_table_[ttkey] = (uint8_t)tricks_takable_by_ns;
  }

  return best_tricks_by_ns;
}

bool Solver::solve_internal_search_plays(
    bool  maximizing,
    int  &alpha,
    int  &beta,
    int  &best_tricks_by_ns,
    Card *best_play
) {
  Cards valid_plays = game_.valid_plays().remove_equivalent_ranks();
  for (auto i = valid_plays.first(); i.valid(); i = valid_plays.next(i)) {
    if (solve_internal_search_single_play(
            i.card(), maximizing, alpha, beta, best_tricks_by_ns, best_play
        )) {
      return true;
    }
  }
  return false;
}

bool Solver::solve_internal_search_single_play(
    Card  c,
    bool  maximizing,
    int  &alpha,
    int  &beta,
    int  &best_tricks_by_ns,
    Card *best_play
) {
  game_.play(c);
  if (tracer_) {
    tracer_->trace_play(c);
  }

  int  child_tricks_by_ns = -1;
  bool prune              = false;

  if (alpha_beta_pruning_enabled_) {
    if (maximizing) {
      int worst_case = game_.tricks_taken_by_ns();
      if (worst_case >= beta) {
        best_tricks_by_ns = worst_case;
        if (best_play) {
          *best_play = c;
        }
        prune = true;
        goto unplay;
      }
    } else {
      int best_case = game_.tricks_taken_by_ns() + game_.tricks_left();
      if (best_case <= alpha) {
        best_tricks_by_ns = best_case;
        if (best_play) {
          *best_play = c;
        }
        prune = true;
        goto unplay;
      }
    }
  }

  child_tricks_by_ns = solve_internal(alpha, beta, nullptr);
  if (maximizing) {
    if (child_tricks_by_ns > best_tricks_by_ns) {
      best_tricks_by_ns = child_tricks_by_ns;
      if (best_play) {
        *best_play = c;
      }
    }
    if (alpha_beta_pruning_enabled_) {
      if (best_tricks_by_ns >= beta) {
        prune = true;
      } else {
        alpha = std::max(alpha, best_tricks_by_ns);
      }
    }
  } else {
    if (child_tricks_by_ns < best_tricks_by_ns) {
      best_tricks_by_ns = child_tricks_by_ns;
      if (best_play) {
        *best_play = c;
      }
    }
    if (alpha_beta_pruning_enabled_) {
      if (best_tricks_by_ns <= alpha) {
        prune = true;
      } else {
        beta = std::min(beta, best_tricks_by_ns);
      }
    }
  }

unplay:
  game_.unplay();
  if (tracer_) {
    tracer_->trace_unplay();
  }

  return prune;
}
