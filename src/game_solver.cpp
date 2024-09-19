#include "game_solver.h"

#include <picosha2.h>

State::State(const Game &g, int alpha_, int beta_, bool normalize) {
  assert(alpha >= 0 && beta >= 0);

  memset(this, 0, sizeof(State));

  alpha = (uint8_t)alpha_;
  beta  = (uint8_t)beta_;

  const Trick &t = g.current_trick();

  Cards to_collapse;
  if (normalize) {
    for (int i = 0; i < 4; i++) {
      to_collapse.add_all(g.hand((Seat)i));
    }
    if (t.started()) {
      for (int i = 0; i < t.card_count(); i++) {
        to_collapse.add(t.card(i));
      }
    }
    to_collapse = to_collapse.complement();
  }

  for (int i = 0; i < 4; i++) {
    hands[i] = g.hand((Seat)i).collapse_ranks(to_collapse).bits();
  }
  if (!t.started()) {
    trick_lead_seat = g.next_seat();
  } else {
    trick_lead_seat  = t.lead_seat();
    trick_card_count = (uint8_t)t.card_count();
    for (int i = 0; i < trick_card_count; i++) {
      Card c   = Cards::collapse_rank(t.card(i), to_collapse);
      trick[i] = (uint8_t)((c.rank() << 4) | c.suit());
    }
  }
}

void State::sha256_hash(uint8_t digest[32]) const {
  uint8_t *in_begin = (uint8_t *)this;
  uint8_t *in_end   = in_begin + sizeof(State);
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
      const State &state,
      int          alpha,
      int          beta,
      int          best_tricks_by_ns
  ) {
    trace_line(game, &state, alpha, beta, "lookup", best_tricks_by_ns);
  }

  void trace_search_start(
      const Game &game, const State &state, int alpha, int beta
  ) {
    trace_line(game, &state, alpha, beta, "start", -1);
  }

  void trace_search_end(
      const Game  &game,
      const State &state,
      int          alpha,
      int          beta,
      bool         alpha_beta_pruned,
      int          best_tricks_by_ns
  ) {
    trace_line(
        game,
        &state,
        alpha,
        beta,
        alpha_beta_pruned ? "end_pruned" : "end",
        best_tricks_by_ns
    );
  }

private:
  void trace_line(
      const Game  &game,
      const State *state,
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
    if (state) {
      uint8_t sha256[32];
      state->sha256_hash(sha256);
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

  State state(game_, alpha, beta, state_normalization_);
  if (!best_play && transposition_table_enabled_) {
    auto it = transposition_table_.find(state);
    if (it != transposition_table_.end()) {
      int best_tricks_by_ns = game_.tricks_taken_by_ns() + it->second;
      if (tracer_) {
        tracer_->trace_table_lookup(
            game_, state, alpha, beta, best_tricks_by_ns
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
    tracer_->trace_search_start(game_, state, alpha, beta);
  }

  Cards valid_plays       = game_.valid_plays();
  bool  alpha_beta_pruned = false;
  for (auto i = valid_plays.first(); i.valid(); i = valid_plays.next(i)) {
    Card c = i.card();
    game_.play(c);

    if (tracer_) {
      tracer_->trace_play(c);
    }

    int tricks_by_ns = solve_internal(alpha, beta, nullptr);
    if (maximizing) {
      if (tricks_by_ns > best_tricks_by_ns) {
        best_tricks_by_ns = tricks_by_ns;
        if (best_play) {
          *best_play = c;
        }
      }
      if (alpha_beta_pruning_enabled_) {
        if (best_tricks_by_ns >= beta) {
          alpha_beta_pruned = true;
          game_.unplay();
          if (tracer_) {
            tracer_->trace_unplay();
          }
          break;
        } else {
          alpha = std::max(alpha, best_tricks_by_ns);
        }
      }
    } else {
      if (tricks_by_ns < best_tricks_by_ns) {
        best_tricks_by_ns = tricks_by_ns;
        if (best_play) {
          *best_play = c;
        }
      }
      if (alpha_beta_pruning_enabled_) {
        if (best_tricks_by_ns <= alpha) {
          alpha_beta_pruned = true;
          game_.unplay();
          if (tracer_) {
            tracer_->trace_unplay();
          }
          break;
        } else {
          beta = std::min(beta, best_tricks_by_ns);
        }
      }
    }

    game_.unplay();
    if (tracer_) {
      tracer_->trace_unplay();
    }
  }

  if (!best_play && transposition_table_enabled_) {
    int tricks_takable_by_ns = best_tricks_by_ns - game_.tricks_taken_by_ns();
    assert(tricks_takable_by_ns >= 0);
    transposition_table_[state] = (uint8_t)tricks_takable_by_ns;
  }

  if (tracer_) {
    tracer_->trace_search_end(
        game_, state, alpha, beta, alpha_beta_pruned, best_tricks_by_ns
    );
  }

  return best_tricks_by_ns;
}
