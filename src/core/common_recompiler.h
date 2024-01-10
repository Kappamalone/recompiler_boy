#pragma once

#include <xbyak/xbyak.h>

using namespace Xbyak::util;
using fp = int (*)();
// using interpreterfp = void (*)(Core&, uint16_t);

// The entire code emitter. God bless xbyak

constexpr int cache_size = 64 * 1024 * 1024;

// If currentCacheSize + cacheLeeway > cacheSize, reset cache
constexpr int cache_leeway = 1024;
uint8_t cache[cache_size]; // emitted code cache
class x64Emitter : public Xbyak::CodeGenerator {
public:
  x64Emitter() : CodeGenerator(cache_size) { // Initialize emitter and memory
    setProtectMode(
        PROTECT_RWE); // Mark emitter memory as readadable/writeable/executable
  }
};

constexpr int pageSize = 32; // size of cache pages
constexpr int pageShift = 5; // shift required to get page froma given address
