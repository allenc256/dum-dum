#pragma once

#include <cassert>
#include <cstdint>

inline int8_t to_int8_t(int x) {
  assert(x >= INT8_MIN && x <= INT8_MAX);
  return (int8_t)x;
}
