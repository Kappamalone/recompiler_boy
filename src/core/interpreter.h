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
  static int ld_r8_r8(Core& core, int r8_src, int r8_dest);
  static int ld_r8_u8(Core& core, int r8_src);
  static int ld_r16_u16(Core& core, int gp1);
  static int ld_a_r16_addr(Core& core, int gp2);
  static int ld_r16_a_addr(Core& core, int gp2);
  static int inc_r8(Core& core, int r8);
  static int dec_r8(Core& core, int r8);
  static int jr_conditional(Core& core, int condition);
};
