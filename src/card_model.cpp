#include <algorithm>
#include <format>
#include <iostream>
#include <sstream>

#include "card_model.h"

static constexpr std::string_view SUIT_STRS[]     = {"♣", "♦", "♥", "♠", "NT"};
static constexpr std::string_view SUIT_STRS_ASC[] = {"C", "D", "H", "S", "NT"};

Suit parse_suit(std::string_view str) {
  Parser parser(str);
  return parse_suit(parser);
}

Suit parse_suit(Parser &parser) {
  for (Suit suit = FIRST_SUIT; suit <= NO_TRUMP; suit++) {
    if (parser.try_parse(SUIT_STRS[suit]) ||
        parser.try_parse(SUIT_STRS_ASC[suit])) {
      return suit;
    }
  }
  throw parser.error("expected suit");
}

const std::string_view &std::formatter<Suit>::to_string(Suit suit) const {
  return SUIT_STRS[suit];
}

static std::string_view RANK_CHARS = "23456789TJQKA";

Rank parse_rank(std::string_view str) {
  Parser p(str);
  return parse_rank(p);
}

std::optional<Rank> try_parse_rank(Parser &parser) {
  for (Rank rank = RANK_2; rank <= ACE; rank++) {
    if (parser.try_parse(RANK_CHARS[rank])) {
      return rank;
    }
  }
  return std::nullopt;
}

Rank parse_rank(Parser &parser) {
  auto rank = try_parse_rank(parser);
  if (rank.has_value()) {
    return *rank;
  } else {
    throw parser.error("expected rank");
  }
}

char std::formatter<Rank>::to_char(Rank rank) const { return RANK_CHARS[rank]; }

static uint8_t make_card_index(Rank rank, Suit suit) {
  return (uint8_t)((rank << 2) | suit);
}

static uint8_t parse_card_index(Parser &parser) {
  Rank rank = parse_rank(parser);
  Suit suit = parse_suit(parser);
  if (suit == NO_TRUMP) {
    throw parser.error("invalid suit (NT)");
  }
  return make_card_index(rank, suit);
}

static uint8_t parse_card_index(std::string_view str) {
  Parser parser(str);
  return parse_card_index(parser);
}

Card::Card(Parser &parser) : index_(parse_card_index(parser)) {}

Card::Card() : index_(0) {}

Card::Card(int card_index) : index_((uint8_t)card_index) {
  assert(card_index >= 0 && card_index < 52);
}

Card::Card(Rank rank, Suit suit) : index_(make_card_index(rank, suit)) {
  assert(suit != NO_TRUMP);
}

Card::Card(std::string_view s) : index_(parse_card_index(s)) {}
Card::Card(const char *s) : Card(std::string_view(s)) {}

Rank Card::rank() const { return (Rank)(index_ >> 2); }
Suit Card::suit() const { return (Suit)(index_ & 0b11); }

Cards::Cards(Parser &parser) : bits_(0) {
  for (Suit suit = LAST_SUIT; suit >= FIRST_SUIT; suit--) {
    if (suit != LAST_SUIT) {
      if (!parser.try_parse('.')) {
        throw parser.error("expected delimiter ('.')");
      }
    }
    while (true) {
      auto rank = try_parse_rank(parser);
      if (rank.has_value()) {
        add(Card(*rank, suit));
      } else {
        break;
      }
    }
  }
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
  Parser parser(s);
  Cards  cards(parser);
  bits_ = cards.bits_;
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
