#pragma once

#include "core.h"

// General calculations:
// CPU executes at 4,194,304 cycles per second
// If we want 60 frames per second, that means we do
// (roughly) 69905 cycles per frame

static constexpr int CYCLES_PER_FRAME = 69905;

class GBInterpreter {
public:
  static int execute_func(Core& core);

  static int jp_u16(Core& core);
  static int ld_r8_r8(Core& core, int r8_dest, int r8_src);
  static int ld_r16_u16(Core& core, int gp1);
};
