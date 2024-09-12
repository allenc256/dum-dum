#pragma once

#include <iostream>

#define PANIC(msg)                   \
  {                                  \
    std::cerr << (msg) << std::endl; \
    exit(1);                         \
  }
