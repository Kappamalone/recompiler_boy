#include "interpreter.h"
#include "core.h"
#include <cstdint>

int GBInterpreter::execute_func(Core& core) {
  int cycles_to_execute = CYCLES_PER_FRAME;

  while (cycles_to_execute > 0) {
    auto opcode = core.mem_read<uint8_t>(core.pc++);
    DPRINT("PC: 0x{:04X}, OPCODE: 0x{:02X}\n", core.pc - 1, opcode);

    int cycles_taken = 0;
    if (opcode == 0x00) {
      // do nothing...
    } else if (opcode == 0b1100'0011) {
      cycles_taken = jp_u16(core);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0x1) {
      cycles_taken = ld_r16_u16(core, (opcode >> 4) & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0010) {
      cycles_taken = ld_r16_a_addr(core, opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1010) {
      cycles_taken = ld_a_r16_addr(core, opcode >> 4 & 0b11);

    } else if (opcode >> 6 == 0b01) {
      cycles_taken = ld_r8_r8(core, opcode >> 3 & 0x7, opcode & 0x7);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b110) {
      cycles_taken = ld_r8_u8(core, opcode >> 3 & 0x7);

    } else {
      PANIC("Unhandled opcode: 0x{:02X} | 0b{:08b}\n", opcode, opcode);
    }

    cycles_to_execute -= cycles_taken;
  }

  return 0;
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

// clang-tidy: disable
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
// clang-tidy: enable

int GBInterpreter::jp_u16(Core& core) {
  auto jump_addr = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;
  core.pc = jump_addr;
  DPRINT("Jumping to 0x{:04X}\n", jump_addr);

  return 16;
}

int GBInterpreter::ld_r16_u16(Core& core, int gp1) {
  auto imm16 = core.mem_read<uint16_t>(core.pc);
  core.pc += 2;

  get_group_1(core, gp1) = imm16;

  return 12;
}

int GBInterpreter::ld_r8_r8(Core& core, int r8_dest, int r8_src) {
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
  auto dest = get_group_2(core, gp2);
  src = dest;
  return 8;
}

int GBInterpreter::ld_r16_a_addr(Core& core, int gp2) {
  auto src = get_group_2(core, gp2);
  auto& dest = get_r8(core, 7);
  src = dest;
  return 8;
}

int GBInterpreter::inc_r8(Core& core, int r8) {
  get_r8(core, r8)++;
  // TODO: getters and setters for flags
  return 8;
}
