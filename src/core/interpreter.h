#pragma once

#include <core.h>

// General calculations:
// CPU executes at 4,194,304 cycles per second
// If we want 60 frames per second, that means we do
// (roughly) 69905 cycles per frame

class core;
static constexpr int CYCLES_PER_FRAME = 69905;

class GBInterpreter {
public:
  static int execute_func(Core& core) {
    int cycles_to_execute = CYCLES_PER_FRAME;
    while (cycles_to_execute > 0) {
      // TODO: actually start interpreting
      core.regs[0] = 0x10;
      cycles_to_execute -= 4;
    }
    return 0;
  }
};
