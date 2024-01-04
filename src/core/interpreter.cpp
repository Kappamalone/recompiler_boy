#include "interpreter.h"
#include "common.h"
#include "core.h"
#include "fmt/core.h"
#include <cstdint>
#include <fstream>
#include <iostream>

static constexpr uint16_t& get_group_1(Core& core, int gp1) {
  switch (gp1) {
    case 0:
      return core.regs[Regs::BC];
      break;
    case 1:
      return core.regs[Regs::DE];
      break;
    case 2:
      return core.regs[Regs::HL];
      break;
    case 3:
      return core.sp;
      break;
    default:
      PANIC("group 1 error!\n");
  }
}

static constexpr uint8_t& get_group_2(Core& core, int gp2, bool write = false) {
  switch (gp2) {
    case 0:
      return core.mem_byte_reference(core.regs[Regs::BC], write);
      break;
    case 1:
      return core.mem_byte_reference(core.regs[Regs::DE], write);
      break;
    case 2:
      core.regs[Regs::HL]++;
      return core.mem_byte_reference(core.regs[Regs::HL] - 1, write);
      break;
    case 3:
      core.regs[Regs::HL]--;
      return core.mem_byte_reference(core.regs[Regs::HL] + 1, write);
      break;
    default:
      PANIC("group 2 error!\n");
  }
}

static constexpr uint16_t& get_group_3(Core& core, int gp3) {
  switch (gp3) {
    case 0:
      return core.regs[Regs::BC];
      break;
    case 1:
      return core.regs[Regs::DE];
      break;
    case 2:
      return core.regs[Regs::HL];
      break;
    case 3:
      core.regs[Regs::AF] &= 0xfff0;
      return core.regs[Regs::AF];
      break;
    default:
      PANIC("group 2 error!\n");
  }
}

// NOLINTBEGIN
static constexpr uint8_t& get_r8(Core& core, int r8, bool write = false) {
  switch (r8) {
    case 0:
      return reinterpret_cast<uint8_t*>(&core.regs[Regs::BC])[1];
      break;
    case 1:
      return reinterpret_cast<uint8_t*>(&core.regs[Regs::BC])[0];
      break;
    case 2:
      return reinterpret_cast<uint8_t*>(&core.regs[Regs::DE])[1];
      break;
    case 3:
      return reinterpret_cast<uint8_t*>(&core.regs[Regs::DE])[0];
      break;
    case 4:
      return reinterpret_cast<uint8_t*>(&core.regs[Regs::HL])[1];
      break;
    case 5:
      return reinterpret_cast<uint8_t*>(&core.regs[Regs::HL])[0];
      break;
    case 6:
      return core.mem_byte_reference(core.regs[Regs::HL], write);
    case 7:
      return reinterpret_cast<uint8_t*>(&core.regs[Regs::AF])[1];
      break;
    default:
      PANIC("r8 error!\n");
  }
}
// NOLINTEND

static constexpr bool condition_table(Core& core, int num) {
  switch (num) {
    case 0:
      return !core.get_flag(Regs::Z);
      break;
    case 1:
      return core.get_flag(Regs::Z);
      break;
    case 2:
      return !core.get_flag(Regs::C);
      break;
    case 3:
      return core.get_flag(Regs::C);
      break;
    default:
      PANIC("condition error!\n");
  }
}

int GBInterpreter::jp_u16(Core& core) {
  auto jump_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;
  core.pc = jump_addr;

  return 0;
}

int GBInterpreter::ld_r16_u16(Core& core, int gp1) {
  auto imm16 = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;

  get_group_1(core, gp1) = imm16;

  return 0;
}

int GBInterpreter::ld_r8_r8(Core& core, int r8_src, int r8_dest) {
  auto& src = get_r8(core, r8_src, true);
  auto& dest = get_r8(core, r8_dest);
  src = dest;
  return 0;
}

int GBInterpreter::ld_r8_u8(Core& core, int r8_src) {
  auto& src = get_r8(core, r8_src, true);
  auto imm8 = core.mem_read<uint8_t>(core.pc++);
  src = imm8;
  return 0;
}

int GBInterpreter::ld_a_r16_addr(Core& core, int gp2) {
  auto& src = get_r8(core, 7, true);
  auto& dest = get_group_2(core, gp2);
  src = dest;
  return 0;
}

int GBInterpreter::ld_r16_a_addr(Core& core, int gp2) {
  auto& src = get_group_2(core, gp2, true);
  auto& dest = get_r8(core, 7);
  src = dest;
  return 0;
}

int GBInterpreter::inc_r8(Core& core, int r8) {
  auto& src = get_r8(core, r8, true);
  core.set_flag(Regs::H, (src & 0xf) == 0xf);
  src++;
  core.set_flag(Regs::Z, src == 0);
  core.set_flag(Regs::N, false);

  return 0;
}

int GBInterpreter::dec_r8(Core& core, int r8) {
  auto& src = get_r8(core, r8, true);
  core.set_flag(Regs::H, (src & 0xf) == 0); // set on borrow...?
  src--;
  core.set_flag(Regs::Z, src == 0);
  core.set_flag(Regs::N, true);

  return 0;
}

// MARK
int GBInterpreter::jr_conditional(Core& core, int condition) {
  auto rel_signed_offest = (int8_t)core.mem_read<uint8_t>(core.pc++);
  if (condition_table(core, condition)) {
    core.pc += rel_signed_offest;
    return 4;
  }

  return 0;
}

int GBInterpreter::di(Core& core) {
  core.IME = false;
  return 0;
}

int GBInterpreter::ei(Core& core) {
  core.req_IME = true;
  return 0;
}

int GBInterpreter::ld_u16_a(Core& core) {
  auto load_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;
  core.mem_byte_reference(load_addr, true) = get_r8(core, 7);
  return 0;
}

int GBInterpreter::ldh_u8_a(Core& core) {
  auto load_addr = core.mem_read<uint8_t>(core.pc++);
  core.mem_byte_reference(0xff00 + load_addr, true) = get_r8(core, 7);
  return 0;
}

int GBInterpreter::call_u16(Core& core) {
  auto jump_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;

  core.sp -= 2;
  core.mem_write<uint16_t>(core.sp, core.pc);

  core.pc = jump_addr;
  return 0;
}

int GBInterpreter::ret(Core& core) {
  auto jump_addr = core.mem_read<uint16_t>(core.sp);
  core.sp += 2;
  core.pc = jump_addr;
  return 0;
}

int GBInterpreter::reti(Core& core) {
  auto jump_addr = core.mem_read<uint16_t>(core.sp);
  core.sp += 2;
  core.pc = jump_addr;
  core.IME = true; // no need to wait a cycle here I guess?
  return 0;
}

int GBInterpreter::rst(Core& core, int vec) {
  auto jump_addr = vec << 3;

  core.sp -= 2;
  core.mem_write<uint16_t>(core.sp, core.pc);

  core.pc = jump_addr;
  return 0;
}

int GBInterpreter::jr_unconditional(Core& core) {
  auto rel_signed_offest = (int8_t)core.mem_read<uint8_t>(core.pc++);
  core.pc += rel_signed_offest;
  return 0;
}

int GBInterpreter::push_r16(Core& core, int gp3) {
  core.sp -= 2;
  core.mem_write<uint16_t>(core.sp, get_group_3(core, gp3));

  return 0;
}

int GBInterpreter::pop_r16(Core& core, int gp3) {
  get_group_3(core, gp3) = core.mem_read<uint16_t>(core.sp);
  core.sp += 2;
  return 0;
}

int GBInterpreter::inc_r16(Core& core, int gp1) {
  get_group_1(core, gp1)++;
  return 0;
}

int GBInterpreter::dec_r16(Core& core, int gp1) {
  get_group_1(core, gp1)--;
  return 0;
}

int GBInterpreter::or_a_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  acc |= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  core.set_flag(Regs::Flag::C, false);
  return 0;
}

int GBInterpreter::ldh_a_u8(Core& core) {
  auto load_addr = core.mem_read<uint8_t>(core.pc++);
  get_r8(core, 7) = core.mem_byte_reference(0xff00 + load_addr);
  return 0;
}

int GBInterpreter::cp_value(Core& core, uint8_t value) {
  auto temp = get_r8(core, 7);
  core.set_flag(Regs::Flag::Z, temp - value == 0);
  core.set_flag(Regs::Flag::N, true);
  core.set_flag(Regs::Flag::H, (uint8_t)((temp & 0xf) - (value & 0xf)) > 0xf);
  core.set_flag(Regs::Flag::C,
                (uint16_t)((uint16_t)temp - (uint16_t)value) > 0xff);
  return 0;
}

int GBInterpreter::ld_a_u16(Core& core) {
  auto value = core.mem_read<uint8_t>(core.mem_read<uint16_t>(core.pc));
  core.pc += 2;
  get_r8(core, 7) = value;
  return 0;
}

int GBInterpreter::and_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  acc &= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, true);
  core.set_flag(Regs::Flag::C, false);

  return 0;
}

// MARK
int GBInterpreter::call_conditional(Core& core, int condition) {
  auto jump_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;

  if (condition_table(core, condition)) {
    core.sp -= 2;
    core.mem_write<uint16_t>(core.sp, core.pc);

    core.pc = jump_addr;
    return 12;
  }
  return 0;
}

int GBInterpreter::xor_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  acc ^= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  core.set_flag(Regs::Flag::C, false);

  return 0;
}

int GBInterpreter::add_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  core.set_flag(Regs::H,
                (uint8_t)(((acc & 0x0f) + (value & 0x0f)) & 0x10) == 0x10);
  core.set_flag(Regs::C, (uint16_t)((uint16_t)acc + (uint16_t)value) > 0xff);
  acc += value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);

  return 0;
}

int GBInterpreter::addc_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  auto carry = core.get_flag(Regs::C);
  core.set_flag(Regs::H, (uint8_t)(((acc & 0x0f) + (value & 0x0f) + carry) &
                                   0x10) == 0x10);
  core.set_flag(Regs::C, (uint16_t)((uint16_t)acc + (uint16_t)value +
                                    (uint16_t)carry) > 0xff);
  acc += value + carry;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);

  return 0;
}

int GBInterpreter::sub_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  core.set_flag(Regs::H, (uint8_t)((acc & 0x0f) - (value & 0x0f)) > 0xf);
  core.set_flag(Regs::C, (uint16_t)((uint16_t)acc - (uint16_t)value) > 0xff);
  acc -= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, true);

  return 0;
}

int GBInterpreter::subc_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  auto carry = core.get_flag(Regs::C);
  core.set_flag(Regs::H,
                (uint8_t)((acc & 0x0f) - (value & 0x0f) - carry) > 0xf);
  core.set_flag(Regs::C, (uint16_t)((uint16_t)acc - (uint16_t)value -
                                    (uint16_t)carry) > 0xff);
  acc = acc - value - carry;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, true);

  return 0;
}

int GBInterpreter::srl(Core& core, uint8_t r8) {
  auto& reg = get_r8(core, r8);
  core.set_flag(Regs::Flag::C, reg & 1);
  reg >>= 1;
  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::rr(Core& core, uint8_t r8) {
  auto& reg = get_r8(core, r8);
  auto carry = core.get_flag(Regs::Flag::C);
  core.set_flag(Regs::Flag::C, reg & 1);

  reg = (reg >> 1) | (carry << 7);

  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::rra(Core& core) {
  auto& reg = get_r8(core, 7);
  auto carry = core.get_flag(Regs::Flag::C);
  core.set_flag(Regs::Flag::C, reg & 1);

  reg = (reg >> 1) | (carry << 7);

  core.set_flag(Regs::Flag::Z, false);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::ret_conditional(Core& core, int condition) {
  if (condition_table(core, condition)) {
    core.pc = core.mem_read<uint16_t>(core.sp);
    core.sp += 2;

    return 12;
  }

  return 0;
}

int GBInterpreter::add_hl_r16(Core& core, int gp1) {
  auto& hl = core.regs[Regs::HL];
  auto& reg = get_group_1(core, gp1);
  core.set_flag(Regs::H,
                (uint16_t)(((hl & 0xfff) + (reg & 0xfff)) & 0x1000) == 0x1000);
  core.set_flag(Regs::C, (uint32_t)((uint32_t)hl + (uint32_t)reg) > 0xffff);
  hl += reg;
  core.set_flag(Regs::Flag::N, false);

  return 0;
}

int GBInterpreter::jp_hl(Core& core) {
  core.pc = core.regs[Regs::HL];

  return 0;
}

int GBInterpreter::or_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  acc |= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  core.set_flag(Regs::Flag::C, false);

  return 0;
}

int GBInterpreter::swap(Core& core, uint8_t r8) {
  auto& reg = get_r8(core, r8);
  auto hi = reg >> 4;
  auto lo = reg & 0xf;
  reg = lo << 4 | hi;
  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  core.set_flag(Regs::Flag::C, false);
  return 0;
}

int GBInterpreter::cpl(Core& core) {
  auto& reg = get_r8(core, 7);
  reg = ~reg;
  core.set_flag(Regs::Flag::N, true);
  core.set_flag(Regs::Flag::H, true);
  return 0;
}

int GBInterpreter::scf(Core& core) {
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  core.set_flag(Regs::Flag::C, true);
  return 0;
}

int GBInterpreter::ccf(Core& core) {
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  core.set_flag(Regs::Flag::C, !core.get_flag(Regs::Flag::C));
  return 0;
}

int GBInterpreter::rlca(Core& core) {
  auto& acc = get_r8(core, 7);
  auto msb = acc >> 7;
  core.set_flag(Regs::Flag::C, msb);
  acc <<= 1;
  acc |= msb;
  core.set_flag(Regs::Flag::Z, false);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::rla_acc(Core& core) {
  auto& acc = get_r8(core, 7);
  auto msb = acc >> 7;
  auto carry = core.get_flag(Regs::Flag::C);
  core.set_flag(Regs::Flag::C, msb);
  acc <<= 1;
  acc |= carry;
  core.set_flag(Regs::Flag::Z, false);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::rrca(Core& core) {
  auto& acc = get_r8(core, 7);
  auto lsb = acc & 1;
  core.set_flag(Regs::Flag::C, lsb);
  acc >>= 1;
  acc |= lsb << 7;
  core.set_flag(Regs::Flag::Z, false);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::rlc(Core& core, int r8) {
  auto& reg = get_r8(core, r8);
  auto msb = reg >> 7;
  core.set_flag(Regs::Flag::C, msb);
  reg <<= 1;
  reg |= msb;
  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::rrc(Core& core, int r8) {
  auto& reg = get_r8(core, r8);
  auto lsb = reg & 1;
  core.set_flag(Regs::Flag::C, lsb);
  reg >>= 1;
  reg |= lsb << 7;
  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::rl(Core& core, uint8_t r8) {
  auto& reg = get_r8(core, r8);
  auto carry = core.get_flag(Regs::Flag::C);
  core.set_flag(Regs::Flag::C, reg >> 7);

  reg = (reg << 1) | carry;

  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::sla(Core& core, uint8_t r8) {
  auto& reg = get_r8(core, r8);
  core.set_flag(Regs::Flag::C, reg >> 7);
  reg = (reg << 1) & 0xFE;
  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::sra(Core& core, uint8_t r8) {
  auto& reg = get_r8(core, r8);
  auto msb = reg >> 7;
  core.set_flag(Regs::Flag::C, reg & 1);
  reg = (reg >> 1) | (msb << 7);
  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::ld_u16_sp(Core& core) {
  core.mem_write<uint16_t>(core.mem_read<uint16_t>(core.pc), core.sp);
  core.pc += 2;
  return 0;
}

int GBInterpreter::ld_sp_hl(Core& core) {
  core.sp = core.regs[Regs::HL];
  return 0;
}

int GBInterpreter::add_sp_i8(Core& core) {
  auto& sp = core.sp;
  uint8_t imm8 = core.mem_read<uint16_t>(core.pc++);
  core.set_flag(Regs::Flag::H, (((sp & 0x0f) + (imm8 & 0x0f) & 0x10)) == 0x10);
  core.set_flag(Regs::Flag::C, (sp & 0xff) + (uint16_t)(imm8) > 0xff);

  sp = (int16_t)core.sp + (int16_t)(int8_t)imm8;

  core.set_flag(Regs::Flag::Z, false);
  core.set_flag(Regs::Flag::N, false);
  return 0;
}

int GBInterpreter::ld_hl_sp_i8(Core& core) {
  auto& hl = core.regs[Regs::HL];
  auto sp = core.sp;
  uint8_t imm8 = core.mem_read<uint16_t>(core.pc++);
  core.set_flag(Regs::Flag::H, (((sp & 0x0f) + (imm8 & 0x0f) & 0x10)) == 0x10);
  core.set_flag(Regs::Flag::C, (sp & 0xff) + (uint16_t)(imm8) > 0xff);

  hl = (int16_t)core.sp + (int16_t)(int8_t)imm8;

  core.set_flag(Regs::Flag::Z, false);
  core.set_flag(Regs::Flag::N, false);
  return 0;
}

int GBInterpreter::bit(Core& core, uint8_t r8, uint8_t bit) {
  core.set_flag(Regs::Flag::Z, !((get_r8(core, r8) >> bit) & 1));
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, true);
  return 0;
}

int GBInterpreter::res(Core& core, uint8_t r8, uint8_t bit) {
  get_r8(core, r8, true) &= ~(1 << bit);
  return 0;
}

int GBInterpreter::set(Core& core, uint8_t r8, uint8_t bit) {
  get_r8(core, r8, true) |= (1 << bit);
  return 0;
}

int GBInterpreter::jp_conditional(Core& core, int condition) {
  auto jump_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;
  if (condition_table(core, condition)) {
    core.pc = jump_addr;
    return 4;
  }
  return 0;
}

int GBInterpreter::daa(Core& core) {
  auto& acc = get_r8(core, 7);
  if (!core.get_flag(Regs::Flag::N)) {
    // previous instruction was addition

    if (core.get_flag(Regs::Flag::C) || acc > 0x99) {
      acc += 0x60;
      core.set_flag(Regs::Flag::C, true);
    }

    if (core.get_flag(Regs::Flag::H) || (acc & 0x0F) > 0x09) {
      acc += 0x06;
    }
  } else {
    // previous instruction was subtraction

    if (core.get_flag(Regs::Flag::C)) {
      acc -= 0x60;
      core.set_flag(Regs::Flag::C, true);
    }

    if (core.get_flag(Regs::Flag::H)) {
      acc -= 0x06;
    }
  }
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::H, false);
  return 0;
}

int GBInterpreter::ld_a_c(Core& core) {
  get_r8(core, 7) = core.mem_read<uint8_t>(0xFF00 + get_r8(core, 1));
  return 0;
}

int GBInterpreter::ld_c_a(Core& core) {
  core.mem_byte_reference(0xFF00 + get_r8(core, 1), true) = get_r8(core, 7);
  return 0;
}

int GBInterpreter::halt(Core& core) {
  core.HALT = true;
  return 0;
}

int GBInterpreter::decode_execute(Core& core, uint16_t opcode) {
  // clang-format off
  static int regularInstructionTiming[256] = {
	1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1,
	1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,
	2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1,
	2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 0, 3, 6, 2, 4,
	2, 3, 3, 0, 3, 4, 2, 4, 2, 4, 3, 0, 3, 0, 2, 4,
	3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4,
	3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4,
  };
  // clang-format on
  int cycles_taken = regularInstructionTiming[opcode] * 4;
  if (opcode == 0x00) {
    // do nothing...
  } else if (opcode == 0b0000'1000) {
    cycles_taken += ld_u16_sp(core);

  } else if (opcode == 0b0001'1000) {
    cycles_taken += jr_unconditional(core);

  } else if (opcode == 0b1110'1010) {
    cycles_taken += ld_u16_a(core);

  } else if ((opcode >> 5) == 0b001 && (opcode & 0x07) == 0b000) {
    cycles_taken += jr_conditional(core, opcode >> 3 & 0b11);

  } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0x1) {
    cycles_taken += ld_r16_u16(core, (opcode >> 4) & 0b11);

  } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1001) {
    cycles_taken += add_hl_r16(core, (opcode >> 4) & 0b11);

  } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0010) {
    cycles_taken += ld_r16_a_addr(core, opcode >> 4 & 0b11);

  } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1010) {
    cycles_taken += ld_a_r16_addr(core, opcode >> 4 & 0b11);

  } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0011) {
    cycles_taken += inc_r16(core, opcode >> 4 & 0b11);

  } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1011) {
    cycles_taken += dec_r16(core, opcode >> 4 & 0b11);

  } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b100) {
    cycles_taken += inc_r8(core, opcode >> 3 & 0x7);

  } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b101) {
    cycles_taken += dec_r8(core, opcode >> 3 & 0x7);

  } else if (opcode == 0b0111'0110) {
    cycles_taken += halt(core);

  } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b110) {
    cycles_taken += ld_r8_u8(core, opcode >> 3 & 0x7);

  } else if (opcode == 0b0010'0111) {
    cycles_taken += daa(core);

  } else if (opcode == 0b0001'1111) {
    cycles_taken += rra(core);

  } else if (opcode == 0b0010'1111) {
    cycles_taken += cpl(core);

  } else if (opcode == 0b0011'0111) {
    cycles_taken += scf(core);

  } else if (opcode == 0b0011'1111) {
    cycles_taken += ccf(core);

  } else if (opcode == 0b0000'0111) {
    cycles_taken += rlca(core);

  } else if (opcode == 0b0001'0111) {
    cycles_taken += rla_acc(core);

  } else if (opcode == 0b0000'1111) {
    cycles_taken += rrca(core);

  } else if (opcode >> 6 == 0b01) {
    cycles_taken += ld_r8_r8(core, opcode >> 3 & 0x7, opcode & 0x7);

  } else if (opcode >> 3 == 0b10110) {
    cycles_taken += or_a_value(core, get_r8(core, opcode & 0x7));

  } else if (opcode >> 3 == 0b10101) {
    cycles_taken += xor_value(core, get_r8(core, opcode & 0x7));

  } else if (opcode >> 3 == 0b10111) {
    cycles_taken += cp_value(core, get_r8(core, opcode & 0x7));

  } else if (opcode >> 3 == 0b10000) {
    cycles_taken += add_value(core, get_r8(core, opcode & 0x7));

  } else if (opcode >> 3 == 0b10001) {
    cycles_taken += addc_value(core, get_r8(core, opcode & 0x7));

  } else if (opcode >> 3 == 0b10010) {
    cycles_taken += sub_value(core, get_r8(core, opcode & 0x7));

  } else if (opcode >> 3 == 0b10011) {
    cycles_taken += subc_value(core, get_r8(core, opcode & 0x7));

  } else if (opcode >> 3 == 0b10100) {
    cycles_taken += and_value(core, get_r8(core, opcode & 0x7));

  } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0) {
    cycles_taken += ret_conditional(core, opcode >> 3 & 0b11);

  } else if (opcode == 0b1110'0000) {
    cycles_taken += ldh_u8_a(core);

  } else if (opcode == 0b1110'1000) {
    cycles_taken += add_sp_i8(core);

  } else if (opcode == 0b1111'0000) {
    cycles_taken += ldh_a_u8(core);

  } else if (opcode == 0b1111'1000) {
    cycles_taken += ld_hl_sp_i8(core);

  } else if (opcode >> 6 == 0b11 && (opcode & 0xf) == 0b0001) {
    cycles_taken += pop_r16(core, opcode >> 4 & 0b11);

  } else if (opcode == 0b1111'1001) {
    cycles_taken += ld_sp_hl(core);

  } else if (opcode == 0b1110'1001) {
    cycles_taken += jp_hl(core);

  } else if (opcode == 0b1100'1001) {
    cycles_taken += ret(core);

  } else if (opcode == 0b1101'1001) {
    cycles_taken += reti(core);

  } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0b010) {
    cycles_taken += jp_conditional(core, opcode >> 3 & 0b11);

  } else if (opcode == 0b1110'0010) {
    cycles_taken += ld_c_a(core);

  } else if (opcode == 0b1111'1010) {
    cycles_taken += ld_a_u16(core);

  } else if (opcode == 0b1111'0010) {
    cycles_taken += ld_a_c(core);

  } else if (opcode == 0b1100'0011) {
    cycles_taken += jp_u16(core);

  } else if (opcode == 0b1111'0011) {
    cycles_taken += di(core);

  } else if (opcode == 0b1111'1011) {
    cycles_taken += ei(core);

  } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0b0100) {
    cycles_taken += call_conditional(core, opcode >> 3 & 0b11);

  } else if (opcode == 0xCB) {
    // clang-format off
    static int extendedInstructionTiming[256] = {
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
	    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
	    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
	    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
	    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    };
    // clang-format on
    const auto second = core.mem_read<uint8_t>(core.pc++);
    if (second >> 3 == 0b00111) {
      srl(core, second & 0x7);

    } else if (second >> 3 == 0b00011) {
      rr(core, second & 0x7);

    } else if (second >> 3 == 0b00110) {
      swap(core, second & 0x7);

    } else if (second >> 3 == 0b00000) {
      rlc(core, second & 0x7);

    } else if (second >> 3 == 0b00001) {
      rrc(core, second & 0x7);

    } else if (second >> 3 == 0b00010) {
      rl(core, second & 0x7);

    } else if (second >> 3 == 0b00100) {
      sla(core, second & 0x7);

    } else if (second >> 3 == 0b00101) {
      sra(core, second & 0x7);

    } else if (second >> 6 == 0b01) {
      bit(core, second & 0x7, (second >> 3) & 0x7);

    } else if (second >> 6 == 0b10) {
      res(core, second & 0x7, (second >> 3) & 0x7);

    } else if (second >> 6 == 0b11) {
      set(core, second & 0x7, (second >> 3) & 0x7);

    } else {
      PANIC("Unhandled bit opcode: 0x{:02X} | 0b{:08b}\n", second, second);
    }
    cycles_taken = extendedInstructionTiming[second] * 4;

  } else if (opcode >> 6 == 0b11 && (opcode & 0xf) == 0b0101) {
    cycles_taken += push_r16(core, opcode >> 4 & 0b11);

  } else if (opcode == 0b1100'1101) {
    cycles_taken += call_u16(core);

  } else if (opcode == 0b1111'1110) {
    cycles_taken += cp_value(core, core.mem_read<uint8_t>(core.pc++));

  } else if (opcode == 0b1110'0110) {
    cycles_taken += and_value(core, core.mem_read<uint8_t>(core.pc++));

  } else if (opcode == 0b1100'0110) {
    cycles_taken += add_value(core, core.mem_read<uint8_t>(core.pc++));

  } else if (opcode == 0b1101'0110) {
    cycles_taken += sub_value(core, core.mem_read<uint8_t>(core.pc++));

  } else if (opcode == 0b1110'1110) {
    cycles_taken += xor_value(core, core.mem_read<uint8_t>(core.pc++));

  } else if (opcode == 0b1100'1110) {
    cycles_taken += addc_value(core, core.mem_read<uint8_t>(core.pc++));

  } else if (opcode == 0b1111'0110) {
    cycles_taken += or_value(core, core.mem_read<uint8_t>(core.pc++));

  } else if (opcode == 0b1101'1110) {
    cycles_taken += subc_value(core, core.mem_read<uint8_t>(core.pc++));

  } else if (opcode >> 6 == 0b11 && (opcode & 0x7) == 0b111) {
    cycles_taken += rst(core, (opcode >> 3) & 0x7);

  } else {
    PANIC("Unhandled opcode: 0x{:02X} | 0b{:08b}\n", opcode, opcode);
  }

  return cycles_taken;
}
