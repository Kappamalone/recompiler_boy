#pragma once

#include <xbyak/xbyak.h>

using namespace Xbyak::util;
using block_fp = int64_t (*)();
// using interpreterfp = void (*)(Core&, uint16_t);
static constexpr int CACHE_SIZE = 512 * 1024 * 1024;

// If current_cache_size + cache_leeway > cache_size, reset cache
static constexpr int CACHE_LEEWAY = 1024;

// The entire code emitter. God bless xbyak

class x64Emitter : public Xbyak::CodeGenerator {
public:
  // emitted code cache
  uint8_t cache[CACHE_SIZE] = {};
  x64Emitter() : CodeGenerator(CACHE_SIZE) { // Initialize emitter and memory
    setProtectMode(
        PROTECT_RWE); // Mark emitter memory as readadable/writeable/executable
  }
};

// size of cache pages
constexpr int PAGE_SIZE = 32;
// shift required to get page from a given address = ctz(page_size)
constexpr int PAGE_SHIFT = 5;

// Register definitions (TODO: make this work with the Windows ABI!!)
const auto RETURN = eax;
const auto PARAM1 = rdi;
const auto PARAM2 = rsi;
const auto PARAM3 = rdx;
const auto SAVED1 = r12;
const auto SAVED2 = r13;
