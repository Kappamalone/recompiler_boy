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
  static int di(Core& core);
  static int ld_u16_a(Core& core);
  static int ldh_u8_a(Core& core);
  static int call_u16(Core& core);
  static int jr_unconditional(Core& core);
  static int ret(Core& core);
  static int push_r16(Core& core, int gp3);
  static int pop_r16(Core& core, int gp3);
  static int inc_r16(Core& core, int gp1);
  static int or_a_value(Core& core, uint8_t value);
  static int ldh_a_u8(Core& core);
  static int cp_value(Core& core, uint8_t value);
  static int ld_a_u16(Core& core);
  static int and_value(Core& core, uint8_t value);
  static int call_conditional(Core& core, int condition);
  static int xor_value(Core& core, uint8_t value);
  static int add_value(Core& core, uint8_t value);
  static int sub_value(Core& core, uint8_t value);
  static int addc_value(Core& core, uint8_t value);
  static int subc_value(Core& core, uint8_t value);
  static int srl(Core& core, uint8_t r8);
  static int rr(Core& core, uint8_t r8);
  static int rra(Core& core);
  static int ret_conditional(Core& core, int condition);
  static int add_hl_r16(Core& core, int gp1);
  static int jp_hl(Core& core);
  static int or_value(Core& core, uint8_t value);
  static int swap(Core& core, uint8_t r8);
};
