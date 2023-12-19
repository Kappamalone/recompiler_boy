#pragma once

#include "fmt/core.h"
#include <cstdint>
#include <vector>

#ifndef NDEBUG
#define DPRINT(...) fmt::print(__VA_ARGS__)
#else
#define DPRINT(...)
#endif

#define PRINT(...) fmt::print(__VA_ARGS__)

// clang-format off
#define PANIC(...) do {     \
  fmt::print(__VA_ARGS__);  \
  exit(1);                  \
} while (0)                 \
//clang-format on
