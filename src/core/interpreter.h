#pragma once

#include "core.h"

// General calculations:
// CPU executes at 4,194,304 cycles per second
// If we want 60 frames per second, that means we do
// (roughly) 69905 cycles per frame

static constexpr int CYCLES_PER_FRAME = 69905;

class GBInterpreter {
public:
  static int decode_execute(Core& core);

  static int jp_u16(Core& core);
  static int ld_r8_r8(Core& core, uint8_t r8_src, uint8_t r8_dest);
  static int ld_r8_u8(Core& core, uint8_t r8_src);
  static int ld_r16_u16(Core& core, uint8_t gp1);
  static int ld_a_r16_addr(Core& core, uint8_t gp2);
  static int ld_r16_a_addr(Core& core, uint8_t gp2);
  static int inc_r8(Core& core, uint8_t r8);
  static int dec_r8(Core& core, uint8_t r8);
  static int jr_conditional(Core& core, uint8_t condition);
  static int di(Core& core);
  static int ei(Core& core);
  static int ld_u16_a(Core& core);
  static int ldh_u8_a(Core& core);
  static int call_u16(Core& core);
  static int jr_unconditional(Core& core);
  static int ret(Core& core);
  static int reti(Core& core);
  static int rst(Core& core, uint8_t vec);
  static int push_r16(Core& core, uint8_t gp3);
  static int pop_r16(Core& core, uint8_t gp3);
  static int inc_r16(Core& core, uint8_t gp1);
  static int dec_r16(Core& core, uint8_t gp1);
  static int or_a_value(Core& core, uint8_t value);
  static int or_a_r8(Core& core, uint8_t r8);
  static int ldh_a_u8(Core& core);
  static int cp_value(Core& core, uint8_t value);
  static int cp_a_value(Core& core, uint8_t r8);
  static int ld_a_u16(Core& core);
  static int and_value(Core& core, uint8_t value);
  static int call_conditional(Core& core, uint8_t condition);
  static int xor_value(Core& core, uint8_t value);
  static int xor_a_r8(Core& core, uint8_t r8);
  static int add_value(Core& core, uint8_t value);
  static int sub_value(Core& core, uint8_t value);
  static int addc_value(Core& core, uint8_t value);
  static int subc_value(Core& core, uint8_t value);
  static int srl(Core& core, uint8_t r8);
  static int rr(Core& core, uint8_t r8);
  static int rra(Core& core);
  static int ret_conditional(Core& core, uint8_t condition);
  static int add_hl_r16(Core& core, uint8_t gp1);
  static int jp_hl(Core& core);
  static int or_value(Core& core, uint8_t value);
  static int swap(Core& core, uint8_t r8);
  static int cpl(Core& core);
  static int scf(Core& core);
  static int ccf(Core& core);
  static int rlca(Core& core);
  static int rla_acc(Core& core);
  static int rrca(Core& core);
  static int rlc(Core& core, uint8_t r8);
  static int rrc(Core& core, uint8_t r8);
  static int rl(Core& core, uint8_t r8);
  static int sla(Core& core, uint8_t r8);
  static int sra(Core& core, uint8_t r8);
  static int ld_u16_sp(Core& core);
  static int ld_sp_hl(Core& core);
  static int add_sp_i8(Core& core);
  static int ld_hl_sp_i8(Core& core);
  static int bit(Core& core, uint8_t r8, uint8_t bit);
  static int res(Core& core, uint8_t r8, uint8_t bit);
  static int set(Core& core, uint8_t r8, uint8_t bit);
  static int jp_conditional(Core& core, uint8_t condition);
  static int daa(Core& core);
  static int ld_a_c(Core& core);
  static int ld_c_a(Core& core);
  static int halt(Core& core);
};
