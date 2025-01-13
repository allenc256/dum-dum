#include "card_model.h"
#include <algorithm>
#include <iostream>
#include <sstream>

static Suit parse_suit(std::istream &is) {
  char ch;
  is >> ch;
  if (!is.good()) {
    throw ParseFailure("stream error");
  }
  switch ((uint8_t)ch) {
  case 'C': return Suit::CLUBS;
  case 'D': return Suit::DIAMONDS;
  case 'H': return Suit::HEARTS;
  case 'S': return Suit::SPADES;
  case 'N':
    if (!is.get(ch) || ch != 'T') {
      throw ParseFailure("bad suit");
    }
    return Suit::NO_TRUMP;
  case 0xE2:
    if (!is.get(ch) || (uint8_t)ch != 0x99) {
      throw ParseFailure("bad suit (utf8 sequence)");
    }
    is.get(ch);
    switch ((uint8_t)ch) {
    case 0xA3: return Suit::CLUBS;
    case 0xA6: return Suit::DIAMONDS;
    case 0xA5: return Suit::HEARTS;
    case 0xA0: return Suit::SPADES;
    }
  }
  throw ParseFailure("bad suit");
}

std::istream &operator>>(std::istream &is, Suit &s) {
  s = parse_suit(is);
  return is;
}

static constexpr std::string_view SUIT_STRS[]     = {"♣", "♦", "♥", "♠", "NT"};
static constexpr std::string_view SUIT_STRS_ASC[] = {"C", "D", "H", "S", "NT"};

std::string_view to_ascii(Suit suit) { return SUIT_STRS_ASC[suit]; }

std::ostream &operator<<(std::ostream &os, Suit s) {
  if (s < 0 || s >= 5) {
    throw std::runtime_error("bad suit");
  }
  os << SUIT_STRS[s];
  return os;
}

static Rank parse_rank(std::istream &is) {
  char ch;
  is >> ch;
  if (!is.good()) {
    throw ParseFailure("stream error");
  }
  if (ch >= '2' && ch <= '9') {
    return (Rank)(ch - '2');
  }
  switch (ch) {
  case 'T': return Rank::TEN;
  case 'J': return Rank::JACK;
  case 'Q': return Rank::QUEEN;
  case 'K': return Rank::KING;
  case 'A': return Rank::ACE;
  default: throw ParseFailure("bad rank");
  }
}

std::istream &operator>>(std::istream &is, Rank &r) {
  r = parse_rank(is);
  return is;
}

static const char *RANK_STRS = "23456789TJQKA";

std::ostream &operator<<(std::ostream &os, Rank r) {
  if (r < 0 || r >= 13) {
    throw std::runtime_error("bad rank");
  }
  os << RANK_STRS[r];
  return os;
}

std::istream &operator>>(std::istream &is, Card &c) {
  Rank r = parse_rank(is);
  Suit s = parse_suit(is);
  if (s == NO_TRUMP) {
    throw ParseFailure("bad suit");
  }
  c = Card(r, s);
  return is;
}

std::ostream &operator<<(std::ostream &os, Card c) {
  os << c.rank() << c.suit();
  return os;
}

Card::Card() : index_(0) {}

Card::Card(int card_index) : index_((uint8_t)card_index) {
  assert(card_index >= 0 && card_index < 52);
}

Card::Card(Rank r, Suit s) : index_((uint8_t)((r << 2) | s)) {
  assert(s != NO_TRUMP);
}

Card::Card(std::string_view s) {
  std::istringstream is(std::string(s), std::ios_base::in);
  is >> *this;
}

Card::Card(const char *s) : Card(std::string_view(s)) {}

Rank Card::rank() const { return (Rank)(index_ >> 2); }
Suit Card::suit() const { return (Suit)(index_ & 0b11); }

std::string Card::to_string() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}

std::ostream &operator<<(std::ostream &os, Cards cards) {
  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    if (suit != LAST_SUIT) {
      os << '.';
    }
    for (Card card : cards.intersect(suit).high_to_low()) {
      os << card.rank();
    }
  }
  return os;
}

bool is_rank_char(int ch) {
  if (ch >= '2' && ch <= '9') {
    return true;
  } else {
    switch (ch) {
    case 'T': return true;
    case 'J': return true;
    case 'Q': return true;
    case 'K': return true;
    case 'A': return true;
    }
  }
  return false;
}

static void parse_cards_ranks(std::istream &is, Suit suit, Cards &cards) {
  while (true) {
    is >> std::ws;
    int ch = is.peek();
    if (is_rank_char(ch)) {
      Rank rank;
      is >> rank;
      cards.add(Card(rank, suit));
    } else {
      break;
    }
  }
}

std::istream &operator>>(std::istream &is, Cards &cards) {
  cards = Cards();
  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    if (suit != LAST_SUIT) {
      char delim = 0;
      is >> delim;
      if (delim != '.') {
        throw ParseFailure("expected delimiter: .");
      }
    }
    parse_cards_ranks(is, suit, cards);
  }
  return is;
}

static const uint64_t SUIT_MASK =
    0b0001000100010001000100010001000100010001000100010001ull;
static const uint64_t ALL_MASK =
    0b1111111111111111111111111111111111111111111111111111ull;

static int      to_card_index(Card c) { return c.rank() * 4 + c.suit(); }
static uint64_t to_card_bit(int card_index) { return 1ull << card_index; }
static uint64_t to_card_bit(Card c) { return to_card_bit(to_card_index(c)); }

Cards::Cards() : bits_(0) {}
Cards::Cards(uint64_t bits) : bits_(bits) { assert(!(bits & ~ALL_MASK)); }

Cards::Cards(std::initializer_list<std::string_view> cards) : bits_(0) {
  for (auto c : cards) {
    add(Card(c));
  }
}

Cards::Cards(std::string_view s) {
  std::istringstream is(std::string(s), std::ios_base::in);
  is >> *this;
}

std::string Cards::to_string() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}

uint64_t Cards::bits() const { return bits_; }
bool     Cards::empty() const { return !bits_; }
void     Cards::add(Card c) { bits_ |= to_card_bit(c); }
void     Cards::add_all(Cards c) { bits_ |= c.bits_; }
void     Cards::remove(Card c) { bits_ &= ~to_card_bit(c); }
void     Cards::remove_all(Cards c) { bits_ &= ~c.bits_; }
bool     Cards::contains(Card c) const { return bits_ & to_card_bit(c); }
bool     Cards::contains_all(Cards c) const { return intersect(c) == c; }
int      Cards::count() const { return std::popcount(bits_); }
Cards    Cards::with(Card c) const { return Cards(bits_ | to_card_bit(c)); }
Cards    Cards::with_all(Cards c) const { return Cards(bits_ | c.bits_); }
Cards    Cards::complement() const { return Cards(~bits_ & ALL_MASK); }
bool     Cards::disjoint(Cards c) const { return intersect(c).empty(); }
Cards    Cards::intersect(Cards c) const { return Cards(bits_ & c.bits_); }
Cards    Cards::without_all(Cards c) const { return Cards(bits_ & ~c.bits_); }

Cards Cards::without_lower(Rank rank) const {
  return Cards(bits_ & (ALL_MASK << (rank * 4)));
}

Cards Cards::intersect(Suit s) const { return Cards(bits_ & (SUIT_MASK << s)); }

Cards Cards::normalize(Cards removed) const {
  if (!removed.bits_) {
    return *this;
  }
  assert(disjoint(removed));
  uint64_t bits = bits_;
  for (int i = 1; i < 13; i++) {
    uint64_t keep_new = (0b1111ull << (i * 4)) & removed.bits_;
    keep_new          = keep_new | (keep_new >> 4);
    keep_new          = keep_new | (keep_new >> 8);
    keep_new          = keep_new | (keep_new >> 16);
    keep_new          = keep_new | (keep_new >> 32);
    uint64_t keep_old = ~keep_new;
    bits              = (bits & keep_old) | (((bits << 4)) & keep_new);
  }
  return Cards(bits);
}

Cards Cards::normalize_wbr(Cards removed) const {
  uint64_t bits = 0;
  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    Cards suit_cards = intersect(suit);
    int   n          = suit_cards.intersect(removed).count();
    bits |= (suit_cards.bits_ << (4 * n)) & ALL_MASK;
  }
  return Cards(bits);
}

Cards Cards::prune_equivalent(Cards removed) const {
  assert(disjoint(removed));

  constexpr uint64_t init_mask =
      0b1111000000000000000000000000000000000000000000000000ull;

  uint64_t bits      = bits_ & init_mask;
  uint64_t next_mask = init_mask >> 4;
  uint64_t prev      = (init_mask & bits_) >> 4;

  for (int i = 0; i < 12; i++) {
    uint64_t next   = next_mask & bits_;
    uint64_t ignore = next_mask & removed.bits_;
    bits |= ~prev & next;
    next_mask >>= 4;
    prev = (next | (prev & ignore)) >> 4;
  }

  return Cards(bits);
}

Cards::Iterable<true> Cards::low_to_high() const {
  return Iterable<true>(bits_);
}

Cards::Iterable<false> Cards::high_to_low() const {
  return Iterable<false>(bits_);
}

Card Cards::lowest() const {
  assert(!empty());
  return *low_to_high().begin();
}

Card Cards::highest() const {
  assert(!empty());
  return *high_to_low().begin();
}

Card Cards::lowest_equivalent(Card card, Cards removed) const {
  uint64_t mask = to_card_bit(card);
  Rank     curr = card.rank();
  Rank     low  = curr;

  while (curr > 0) {
    curr--;
    mask >>= 4;
    bool pres_here      = mask & bits_;
    bool pres_elsewhere = !(mask & removed.bits_);
    if (pres_here) {
      low = curr;
    } else if (pres_elsewhere) {
      break;
    }
  }

  return Card(low, card.suit());
}

Cards Cards::all() { return Cards(ALL_MASK); }
Cards Cards::all(Suit s) { return Cards(SUIT_MASK << s); }

Cards Cards::higher_ranking(Card card) {
  uint64_t rank_bits = (SUIT_MASK << ((card.rank() + 1) * 4)) & ALL_MASK;
  return Cards(rank_bits << card.suit());
}

Cards Cards::higher_ranking_or_eq(Card card) {
  uint64_t rank_bits = (SUIT_MASK << (card.rank() * 4)) & ALL_MASK;
  return Cards(rank_bits << card.suit());
}

Cards Cards::lower_ranking(Card card) {
  uint64_t rank_bits = (SUIT_MASK >> ((13 - card.rank()) * 4)) & ALL_MASK;
  return Cards(rank_bits << card.suit());
}

static constexpr uint64_t SUIT_NORM_ONES      = 0x0001111111111111ull;
static constexpr uint64_t SUIT_NORM_IDENT_MAP = 0x000cba9876543210ull;
static constexpr uint64_t SUIT_NORM_MASK      = 0x000fffffffffffffull;

SuitNormalizer::SuitNormalizer()
    : norm_map_(SUIT_NORM_IDENT_MAP),
      denorm_map_(SUIT_NORM_IDENT_MAP) {}

Rank SuitNormalizer::normalize(Rank rank) const {
  return (Rank)((norm_map_ >> (rank * 4)) & 0b1111);
}

Rank SuitNormalizer::denormalize(Rank rank) const {
  return (Rank)((denorm_map_ >> (rank * 4)) & 0b1111);
}

void SuitNormalizer::remove(Rank rank) {
  Rank     nr = normalize(rank);
  uint64_t m  = SUIT_NORM_MASK >> ((12 - rank) * 4);
  uint64_t nm = SUIT_NORM_MASK >> ((12 - nr) * 4);
  norm_map_ += SUIT_NORM_ONES & m;
  denorm_map_ = (denorm_map_ & ~nm) | (((denorm_map_ & nm) << 4) & nm);
}

void SuitNormalizer::add(Rank rank) {
  uint64_t m = SUIT_NORM_MASK >> ((12 - rank) * 4);
  norm_map_ -= SUIT_NORM_ONES & m;
  Rank     nr = normalize(rank);
  uint64_t nm = SUIT_NORM_MASK >> ((12 - nr) * 4);
  denorm_map_ = (denorm_map_ & ~nm) | ((denorm_map_ & nm) >> 4) |
                ((uint64_t)rank << (nr * 4));
}

Card CardNormalizer::normalize(Card card) const {
  Rank r = norm_[card.suit()].normalize(card.rank());
  return Card(r, card.suit());
}

Card CardNormalizer::denormalize(Card card) const {
  Rank r = norm_[card.suit()].denormalize(card.rank());
  return Card(r, card.suit());
}

Cards CardNormalizer::normalize(Cards cards) const {
  return cards.normalize(removed_);
}

Cards CardNormalizer::normalize_wbr(Cards cards) const {
  return cards.normalize_wbr(removed_);
}

Cards CardNormalizer::denormalize_wbr(Cards cards) const {
  Cards result;
  for (Suit suit = FIRST_SUIT; suit <= LAST_SUIT; suit++) {
    Cards suit_cards = cards.intersect(suit);
    if (!suit_cards.empty()) {
      result.add_all(Cards::higher_ranking_or_eq(denormalize(suit_cards.lowest()
      )));
    }
  }
  return result;
}

Cards CardNormalizer::prune_equivalent(Cards cards) const {
  return cards.prune_equivalent(removed_);
}

void CardNormalizer::remove(Card card) {
  assert(!removed_.contains(card));
  removed_.add(card);
  norm_[card.suit()].remove(card.rank());
}

void CardNormalizer::add(Card card) {
  assert(removed_.contains(card));
  removed_.remove(card);
  norm_[card.suit()].add(card.rank());
}

void CardNormalizer::remove_all(Cards cards) {
  for (Card c : cards.high_to_low()) {
    remove(c);
  }
}
