#include <iostream>

#include "Card.h"

int main(int argc, char **argv) {
  Cards c;
  c = c.with_card(Card("2C"));
  c = c.with_card(Card("2H"));
  c = c.with_card(Card("3H"));
  c = c.with_card(Card("AH"));
  c = c.with_card(Card("TS"));
  c = c.with_card(Card("AS"));
  // std::cout << std::countl_zero((uint64_t)0) << std::endl;
  std::cout << c << std::endl;
  std::cout << sizeof(c) << std::endl;
  return 0;
}
