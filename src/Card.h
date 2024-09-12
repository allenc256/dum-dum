#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <ostream>

class Card {
 public:
  enum Suit { C, D, H, S };
  enum Rank { R2, R3, R4, R5, R6, R7, R8, R9, T, J, Q, K, A };

  Card(Rank rank, Suit suit);
  Card(std::string_view utf8_str);

  Suit GetSuit() const;
  Rank GetRank() const;

 private:
  uint64_t _card;

  friend std::ostream& operator<<(std::ostream& os, const Card& c);
};

std::ostream& operator<<(std::ostream& os, const Card::Suit& s);
std::ostream& operator<<(std::ostream& os, const Card::Rank& r);

inline Card::Card(Rank rank, Suit suit)
    : _card(((uint64_t)1) << (rank + suit * 13)) {}

inline Card::Suit Card::GetSuit() const {
  auto s = std::countr_zero(_card) / 13;
  assert(s >= 0 & s < 4);
  return static_cast<Suit>(s);
}

inline Card::Rank Card::GetRank() const {
  return static_cast<Rank>(std::countr_zero(_card) % 13);
}
