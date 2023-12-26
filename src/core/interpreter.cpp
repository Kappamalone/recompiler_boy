#include "interpreter.h"
#include "core.h"
#include "fmt/core.h"
#include <cstdint>
#include <fstream>
#include <iostream>

void append_to_logging(const std::string& content) {
  std::ofstream file("../../logging.txt", std::ios::app);

  if (file.is_open()) {
    file << content;
    file.close();
  } else {
    std::cerr << "Unable to open file: " << std::endl;
  }
}

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

static constexpr uint8_t& get_group_2(Core& core, int gp2) {
  switch (gp2) {
    case 0:
      return core.mem_byte_reference(core.regs[Regs::BC]);
      break;
    case 1:
      return core.mem_byte_reference(core.regs[Regs::DE]);
      break;
    case 2:
      core.regs[Regs::HL]++;
      return core.mem_byte_reference(core.regs[Regs::HL] - 1);
      break;
    case 3:
      core.regs[Regs::HL]--;
      return core.mem_byte_reference(core.regs[Regs::HL] + 1);
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
      return core.regs[Regs::AF];
      break;
    default:
      PANIC("group 2 error!\n");
  }
}

// NOLINTBEGIN
static constexpr uint8_t& get_r8(Core& core, int r8) {
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
      return core.mem_byte_reference(core.regs[Regs::HL]);
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

int GBInterpreter::execute_func(Core& core) {
  int cycles_to_execute = CYCLES_PER_FRAME;

  while (cycles_to_execute > 0) {
    append_to_logging(fmt::format(
        "A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: "
        "{:02X} E: {:02X} H: {:02X} L: {:02X} SP: {:04X} PC: "
        "00:{:04X} ({:02X} "
        "{:02X} {:02X} {:02X})\n",
        core.regs[Regs::AF] >> 8, core.regs[Regs::AF] & 0xff,
        core.regs[Regs::BC] >> 8, core.regs[Regs::BC] & 0xff,
        core.regs[Regs::DE] >> 8, core.regs[Regs::DE] & 0xff,
        core.regs[Regs::HL] >> 8, core.regs[Regs::HL] & 0xff, core.sp, core.pc,
        core.mem_read<uint8_t>(core.pc), core.mem_read<uint8_t>(core.pc + 1),
        core.mem_read<uint8_t>(core.pc + 2),
        core.mem_read<uint8_t>(core.pc + 3)));
    auto opcode = core.mem_read<uint8_t>(core.pc);
    core.pc++;
    // DPRINT("PC: 0x{:04X}, OPCODE: 0x{:02X}\n", core.pc - 1, opcode);

    int cycles_taken = 0;
    if (opcode == 0x00) {
      // do nothing...
      cycles_taken = 4;
    } else if (opcode == 0b0001'1000) {
      cycles_taken = jr_unconditional(core);

    } else if (opcode == 0b1110'1010) {
      cycles_taken = ld_u16_a(core);

    } else if ((opcode >> 5) == 0b001 && (opcode & 0x07) == 0b000) {
      cycles_taken = jr_conditional(core, opcode >> 3 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0x1) {
      cycles_taken = ld_r16_u16(core, (opcode >> 4) & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1001) {
      cycles_taken = add_hl_r16(core, (opcode >> 4) & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0010) {
      cycles_taken = ld_r16_a_addr(core, opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1010) {
      cycles_taken = ld_a_r16_addr(core, opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0011) {
      cycles_taken = inc_r16(core, opcode >> 4 & 0b11);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b100) {
      cycles_taken = inc_r8(core, opcode >> 3 & 0x7);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b101) {
      cycles_taken = dec_r8(core, opcode >> 3 & 0x7);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b110) {
      cycles_taken = ld_r8_u8(core, opcode >> 3 & 0x7);

    } else if (opcode == 0b0001'1111) {
      cycles_taken = rra(core);

    } else if (opcode >> 6 == 0b01) {
      cycles_taken = ld_r8_r8(core, opcode >> 3 & 0x7, opcode & 0x7);

    } else if (opcode >> 3 == 0b10110) {
      cycles_taken = or_a_value(core, get_r8(core, opcode & 0x7));

    } else if (opcode >> 3 == 0b10101) {
      cycles_taken = xor_value(core, get_r8(core, opcode & 0x7));

    } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0) {
      cycles_taken = ret_conditional(core, opcode >> 3 & 0b11);

    } else if (opcode == 0b1110'0000) {
      cycles_taken = ldh_u8_a(core);

    } else if (opcode == 0b1111'0000) {
      cycles_taken = ldh_a_u8(core);

    } else if (opcode >> 6 == 0b11 && (opcode & 0xf) == 0b0001) {
      cycles_taken = pop_r16(core, opcode >> 4 & 0b11);

    } else if (opcode == 0b1110'1001) {
      cycles_taken = jp_hl(core);

    } else if (opcode == 0b1100'1001) {
      cycles_taken = ret(core);

    } else if (opcode == 0b1111'1010) {
      cycles_taken = ld_a_u16(core);

    } else if (opcode == 0b1100'0011) {
      cycles_taken = jp_u16(core);

    } else if (opcode == 0b1111'0011) {
      cycles_taken = di(core);

    } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0b0100) {
      cycles_taken = call_conditional(core, opcode >> 3 & 0b11);

    } else if (opcode == 0xCB) {
      const auto bit = core.mem_read<uint8_t>(core.pc++);
      if (bit >> 3 == 0b00111) {
        cycles_taken = srl(core, bit & 0x7);

      } else if (bit >> 3 == 0b00011) {
        cycles_taken = rr(core, bit & 0x7);

      } else {
        PANIC("Unhandled bit opcode: 0x{:02X} | 0b{:08b}\n", bit, bit);
      }

    } else if (opcode >> 6 == 0b11 && (opcode & 0xf) == 0b0101) {
      cycles_taken = push_r16(core, opcode >> 4 & 0b11);

    } else if (opcode == 0b1100'1101) {
      cycles_taken = call_u16(core);

    } else if (opcode == 0b1111'1110) {
      cycles_taken = cp_value(core, core.mem_read<uint8_t>(core.pc++));

    } else if (opcode == 0b1110'0110) {
      cycles_taken = and_value(core, core.mem_read<uint8_t>(core.pc++));

    } else if (opcode == 0b1100'0110) {
      cycles_taken = add_value(core, core.mem_read<uint8_t>(core.pc++));

    } else if (opcode == 0b1101'0110) {
      cycles_taken = sub_value(core, core.mem_read<uint8_t>(core.pc++));

    } else if (opcode == 0b1110'1110) {
      cycles_taken = xor_value(core, core.mem_read<uint8_t>(core.pc++));

    } else if (opcode == 0b1100'1110) {
      cycles_taken = add_value(core, core.mem_read<uint8_t>(core.pc++) +
                                         core.get_flag(Regs::Flag::C));

    } else {
      PANIC("Unhandled opcode: 0x{:02X} | 0b{:08b}\n", opcode, opcode);
    }

    cycles_to_execute -= cycles_taken;
  }

  return 0;
}

int GBInterpreter::jp_u16(Core& core) {
  auto jump_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;
  core.pc = jump_addr;

  return 16;
}

int GBInterpreter::ld_r16_u16(Core& core, int gp1) {
  auto imm16 = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;

  get_group_1(core, gp1) = imm16;

  return 12;
}

int GBInterpreter::ld_r8_r8(Core& core, int r8_src, int r8_dest) {
  auto& src = get_r8(core, r8_src);
  auto& dest = get_r8(core, r8_dest);
  src = dest;
  return 4;
}

int GBInterpreter::ld_r8_u8(Core& core, int r8_src) {
  auto& src = get_r8(core, r8_src);
  auto imm8 = core.mem_read<uint8_t>(core.pc++);
  src = imm8;
  return 4;
}

int GBInterpreter::ld_a_r16_addr(Core& core, int gp2) {
  auto& src = get_r8(core, 7);
  auto& dest = get_group_2(core, gp2);
  src = dest;
  return 8;
}

int GBInterpreter::ld_r16_a_addr(Core& core, int gp2) {
  auto& src = get_group_2(core, gp2);
  auto& dest = get_r8(core, 7);
  src = dest;
  return 8;
}

int GBInterpreter::inc_r8(Core& core, int r8) {
  auto& src = get_r8(core, r8);
  core.set_flag(Regs::H, (src & 0xf) == 0xf);
  src++;
  core.set_flag(Regs::Z, src == 0);
  core.set_flag(Regs::N, false);

  return 4;
}

int GBInterpreter::dec_r8(Core& core, int r8) {
  auto& src = get_r8(core, r8);
  core.set_flag(Regs::H, (src & 0xf) == 0); // set on borrow...?
  src--;
  core.set_flag(Regs::Z, src == 0);
  core.set_flag(Regs::N, true);

  return 4;
}

int GBInterpreter::jr_conditional(Core& core, int condition) {
  auto rel_signed_offest = (int8_t)core.mem_read<uint8_t>(core.pc++);
  if (condition_table(core, condition)) {
    core.pc += rel_signed_offest;
    return 12;
  }

  return 8;
}

int GBInterpreter::di(Core& core) {
  // TODO:
  return 4;
}

int GBInterpreter::ld_u16_a(Core& core) {
  auto load_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;
  core.mem_byte_reference(load_addr) = get_r8(core, 7);
  return 16;
}

int GBInterpreter::ldh_u8_a(Core& core) {
  auto load_addr = core.mem_read<uint8_t>(core.pc++);
  core.mem_byte_reference(0xff00 + load_addr) = get_r8(core, 7);
  return 12;
}

int GBInterpreter::call_u16(Core& core) {
  auto jump_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;

  core.sp -= 2;
  core.mem_write<uint16_t>(core.sp, core.pc);

  core.pc = jump_addr;
  return 24;
}

int GBInterpreter::ret(Core& core) {
  auto jump_addr = core.mem_read<uint16_t>(core.sp);
  core.sp += 2;
  core.pc = jump_addr;
  return 16;
}

int GBInterpreter::jr_unconditional(Core& core) {
  auto rel_signed_offest = (int8_t)core.mem_read<uint8_t>(core.pc++);
  core.pc += rel_signed_offest;
  return 12;
}

int GBInterpreter::push_r16(Core& core, int gp3) {
  core.sp -= 2;
  core.mem_write<uint16_t>(core.sp, get_group_3(core, gp3));

  return 16;
}

int GBInterpreter::pop_r16(Core& core, int gp3) {
  get_group_3(core, gp3) = core.mem_read<uint16_t>(core.sp);
  core.sp += 2;
  return 12;
}

int GBInterpreter::inc_r16(Core& core, int gp1) {
  get_group_1(core, gp1)++;
  return 8;
}

int GBInterpreter::or_a_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  acc |= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  core.set_flag(Regs::Flag::C, false);
  return 4;
}

int GBInterpreter::ldh_a_u8(Core& core) {
  auto load_addr = core.mem_read<uint8_t>(core.pc++);
  get_r8(core, 7) = core.mem_byte_reference(0xff00 + load_addr);
  return 12;
}

int GBInterpreter::cp_value(Core& core, uint8_t value) {
  auto temp = get_r8(core, 7);
  core.set_flag(Regs::Flag::Z, temp - value == 0);
  core.set_flag(Regs::Flag::N, true);
  core.set_flag(Regs::Flag::H, (uint8_t)((temp & 0xf) - (value & 0xf)) > 0xf);
  core.set_flag(Regs::Flag::C,
                (uint16_t)((uint16_t)temp - (uint16_t)value) > 0xff);
  return 8;
}

int GBInterpreter::ld_a_u16(Core& core) {
  auto value = core.mem_read<uint8_t>(core.mem_read<uint16_t>(core.pc));
  core.pc += 2;
  get_r8(core, 7) = value;
  return 16;
}

int GBInterpreter::and_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  acc &= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, true);
  core.set_flag(Regs::Flag::C, false);

  return 4;
}

int GBInterpreter::call_conditional(Core& core, int condition) {
  auto jump_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;

  if (condition_table(core, condition)) {
    core.sp -= 2;
    core.mem_write<uint16_t>(core.sp, core.pc);

    core.pc = jump_addr;
    return 24;
  }
  return 12;
}

int GBInterpreter::xor_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  acc ^= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  core.set_flag(Regs::Flag::C, false);

  return 4;
}

int GBInterpreter::add_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  core.set_flag(Regs::H,
                (uint8_t)(((acc & 0x0f) + (value & 0x0f)) & 0x10) == 0x10);
  core.set_flag(Regs::C, (uint16_t)((uint16_t)acc + (uint16_t)value) > 0xff);
  acc += value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, false);

  return 8;
}

int GBInterpreter::sub_value(Core& core, uint8_t value) {
  auto& acc = get_r8(core, 7);
  core.set_flag(Regs::H, (uint8_t)((acc & 0x0f) - (value & 0x0f)) > 0xf);
  core.set_flag(Regs::C, (uint16_t)((uint16_t)acc - (uint16_t)value) > 0xff);
  acc -= value;
  core.set_flag(Regs::Flag::Z, acc == 0);
  core.set_flag(Regs::Flag::N, true);

  return 8;
}

int GBInterpreter::srl(Core& core, uint8_t r8) {
  auto& reg = get_r8(core, r8);
  core.set_flag(Regs::Flag::C, reg & 1);
  reg >>= 1;
  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 8;
}

int GBInterpreter::rr(Core& core, uint8_t r8) {
  auto& reg = get_r8(core, r8);
  auto carry = core.get_flag(Regs::Flag::C);
  core.set_flag(Regs::Flag::C, reg & 1);

  reg = (reg >> 1) | (carry << 7);

  core.set_flag(Regs::Flag::Z, reg == 0);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 8;
}

int GBInterpreter::rra(Core& core) {
  auto& reg = get_r8(core, 7);
  auto carry = core.get_flag(Regs::Flag::C);
  core.set_flag(Regs::Flag::C, reg & 1);

  reg = (reg >> 1) | (carry << 7);

  core.set_flag(Regs::Flag::Z, false);
  core.set_flag(Regs::Flag::N, false);
  core.set_flag(Regs::Flag::H, false);
  return 8;
}

int GBInterpreter::ret_conditional(Core& core, int condition) {
  if (condition_table(core, condition)) {
    core.pc = core.mem_read<uint16_t>(core.sp);
    core.sp += 2;

    return 20;
  }

  return 8;
}

int GBInterpreter::add_hl_r16(Core& core, int gp1) {
  auto& hl = core.regs[Regs::HL];
  auto& reg = get_group_1(core, gp1);
  core.set_flag(Regs::H,
                (uint16_t)(((hl & 0xfff) + (reg & 0xfff)) & 0x1000) == 0x1000);
  core.set_flag(Regs::C, (uint32_t)((uint32_t)hl + (uint32_t)reg) > 0xffff);
  hl += reg;
  core.set_flag(Regs::Flag::N, false);

  return 8;
}

int GBInterpreter::jp_hl(Core& core) {
  core.pc = core.regs[Regs::HL];

  return 4;
}
