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
//

#define PRINT8(VAR) PRINT("0x{:02X}\n", VAR);
#define PRINT16(VAR) PRINT("0x{:04X}\n", VAR);
#define PRINT32(VAR) PRINT("0x{:08X}\n", VAR);
#define PRINT64(VAR) PRINT("0x{:016X}\n", VAR);

// bit manipulation
#define BIT(VALUE, POS) ((VALUE >> POS) & 1)
#define BIT_REGION(VALUE, POS, SIZE) (VALUE >> POS) & ((((uint64_t)1 << SIZE)-1))
