#include "cached_interpreter.h"
#include "common_recompiler.h"
#include "interpreter.h"

// NOLINTBEGIN
template <bool Write = false>
static constexpr uint8_t& get_r8(Core& core, int r8, uint8_t value = 0) {
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
      return core.mem_byte_reference<Write>(core.regs[Regs::HL], value);
    case 7:
      return reinterpret_cast<uint8_t*>(&core.regs[Regs::AF])[1];
      break;
    default:
      PANIC("r8 error!\n");
  }
}
// NOLINTEND

void GBCachedInterpreter::emit_prologue(Core& core) {
  // prologue
  code.push(rbp);
  code.mov(rbp, rsp);
  code.push(SAVED1);
}

void GBCachedInterpreter::emit_epilogue(Core& core) {
  // epilogue
  code.pop(SAVED1);
  code.leave();
  code.ret();
}

void GBCachedInterpreter::emit_fallback_no_params(no_params_fp fallback,
                                                  Core& core) {
  code.mov(rax, (uintptr_t)fallback);
  code.mov(PARAM1, (uintptr_t)&core);
  code.call(rax);
}

void GBCachedInterpreter::emit_fallback_one_params(one_params_fp fallback,
                                                   Core& core, int first) {
  code.mov(rax, (uintptr_t)fallback);
  code.mov(PARAM1, (uintptr_t)&core);
  code.mov(PARAM2, first);
  code.call(rax);
}

void GBCachedInterpreter::emit_fallback_two_params(two_params_fp fallback,
                                                   Core& core, int first,
                                                   int second) {
  code.mov(rax, (uintptr_t)fallback);
  code.mov(PARAM1, (uintptr_t)&core);
  code.mov(PARAM2, first);
  code.mov(PARAM3, second);
  code.call(rax);
}

block_fp GBCachedInterpreter::recompile_block(Core& core) {
  check_emitted_cache();
  // TODO: block invalidation

  auto emitted_function = (block_fp)code.getCurr();
  auto dyn_pc = core.pc;
  auto static_cycles_taken = 0;
  bool jump_emitted = false;

  emit_prologue(core);
  // SAVED1 will hold all dynamically emitted cycles
  code.mov(SAVED1, 0);

  // At compile time, we know what static cycles to add onto the
  // PC. However, we still have to account for conditional cycles

  while (true) {
    PRINT("DYN PC: 0x{:04X}\n", dyn_pc);
    const auto opcode = core.mem_read<uint8_t>(dyn_pc++);
    const auto initial_dyn_pc = dyn_pc;
    if (opcode != 0xCB) {
      static_cycles_taken += regular_instr_timing[opcode] * 4;
    }

    if (opcode == 0x00) {
      // do nothing...
    } else if (opcode == 0x10) {
      dyn_pc++;

    } else if (opcode == 0b0000'1000) {
      emit_fallback_no_params(GBInterpreter::ld_u16_sp, core);
      dyn_pc += 2;

    } else if (opcode == 0b0001'1000) {
      emit_fallback_no_params(GBInterpreter::jr_unconditional, core);
      jump_emitted = true;

    } else if (opcode == 0b1110'1010) {
      emit_fallback_no_params(GBInterpreter::ld_u16_a, core);
      dyn_pc += 2;

    } else if ((opcode >> 5) == 0b001 && (opcode & 0x07) == 0b000) {
      emit_fallback_one_params(GBInterpreter::jr_conditional, core,
                               opcode >> 3 & 0b11);
      jump_emitted = true;

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0x1) {
      emit_fallback_one_params(GBInterpreter::ld_r16_u16, core,
                               opcode >> 4 & 0b11);
      dyn_pc += 2;

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1001) {
      emit_fallback_one_params(GBInterpreter::add_hl_r16, core,
                               opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0010) {
      emit_fallback_one_params(GBInterpreter::ld_r16_a_addr, core,
                               opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1010) {
      emit_fallback_one_params(GBInterpreter::ld_a_r16_addr, core,
                               opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0011) {
      emit_fallback_one_params(GBInterpreter::inc_r16, core,
                               opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1011) {
      emit_fallback_one_params(GBInterpreter::dec_r16, core,
                               opcode >> 4 & 0b11);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b100) {
      emit_fallback_one_params(GBInterpreter::inc_r8, core, opcode >> 3 & 0x7);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b101) {
      emit_fallback_one_params(GBInterpreter::dec_r8, core, opcode >> 3 & 0x7);

    } else if (opcode == 0b0111'0110) {
      PANIC("how to handle halt?");
      emit_fallback_no_params(GBInterpreter::halt, core);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b110) {
      emit_fallback_one_params(GBInterpreter::ld_r8_u8, core,
                               opcode >> 3 & 0x7);
      dyn_pc++;

    } else if (opcode == 0b0010'0111) {
      emit_fallback_no_params(GBInterpreter::daa, core);

    } else if (opcode == 0b0001'1111) {
      emit_fallback_no_params(GBInterpreter::rra, core);

    } else if (opcode == 0b0010'1111) {
      emit_fallback_no_params(GBInterpreter::cpl, core);

    } else if (opcode == 0b0011'0111) {
      emit_fallback_no_params(GBInterpreter::scf, core);

    } else if (opcode == 0b0011'1111) {
      emit_fallback_no_params(GBInterpreter::ccf, core);

    } else if (opcode == 0b0000'0111) {
      emit_fallback_no_params(GBInterpreter::rlca, core);

    } else if (opcode == 0b0001'0111) {
      emit_fallback_no_params(GBInterpreter::rla_acc, core);

    } else if (opcode == 0b0000'1111) {
      emit_fallback_no_params(GBInterpreter::rrca, core);

    } else if (opcode >> 6 == 0b01) {
      emit_fallback_two_params(GBInterpreter::ld_r8_r8, core, opcode >> 3 & 0x7,
                               opcode & 0x7);

    } else if (opcode >> 3 == 0b10110) {
      emit_fallback_one_params(GBInterpreter::or_a_value, core,
                               get_r8(core, opcode & 0x7));

    } else if (opcode >> 3 == 0b10101) {
      emit_fallback_one_params(GBInterpreter::xor_value, core,
                               get_r8(core, opcode & 0x7));

    } else if (opcode >> 3 == 0b10111) {
      emit_fallback_one_params(GBInterpreter::cp_value, core,
                               get_r8(core, opcode & 0x7));

    } else if (opcode >> 3 == 0b10000) {
      emit_fallback_one_params(GBInterpreter::add_value, core,
                               get_r8(core, opcode & 0x7));

    } else if (opcode >> 3 == 0b10001) {
      emit_fallback_one_params(GBInterpreter::addc_value, core,
                               get_r8(core, opcode & 0x7));

    } else if (opcode >> 3 == 0b10010) {
      emit_fallback_one_params(GBInterpreter::sub_value, core,
                               get_r8(core, opcode & 0x7));

    } else if (opcode >> 3 == 0b10011) {
      emit_fallback_one_params(GBInterpreter::subc_value, core,
                               get_r8(core, opcode & 0x7));

    } else if (opcode >> 3 == 0b10100) {
      emit_fallback_one_params(GBInterpreter::and_value, core,
                               get_r8(core, opcode & 0x7));

    } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0) {
      emit_fallback_one_params(GBInterpreter::ret_conditional, core,
                               opcode >> 3 & 0b11);
      jump_emitted = true;

    } else if (opcode == 0b1110'0000) {
      emit_fallback_no_params(GBInterpreter::ldh_u8_a, core);
      dyn_pc++;

    } else if (opcode == 0b1110'1000) {
      emit_fallback_no_params(GBInterpreter::add_sp_i8, core);
      dyn_pc++;

    } else if (opcode == 0b1111'0000) {
      emit_fallback_no_params(GBInterpreter::ldh_a_u8, core);
      dyn_pc++;

    } else if (opcode == 0b1111'1000) {
      emit_fallback_no_params(GBInterpreter::ld_hl_sp_i8, core);
      dyn_pc++;

    } else if (opcode >> 6 == 0b11 && (opcode & 0xf) == 0b0001) {
      emit_fallback_one_params(GBInterpreter::pop_r16, core,
                               opcode >> 4 & 0b11);

    } else if (opcode == 0b1111'1001) {
      emit_fallback_no_params(GBInterpreter::ld_sp_hl, core);

    } else if (opcode == 0b1110'1001) {
      emit_fallback_no_params(GBInterpreter::jp_hl, core);
      jump_emitted = true;

    } else if (opcode == 0b1100'1001) {
      emit_fallback_no_params(GBInterpreter::ret, core);
      jump_emitted = true;

    } else if (opcode == 0b1101'1001) {
      emit_fallback_no_params(GBInterpreter::reti, core);
      jump_emitted = true;

    } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0b010) {
      emit_fallback_one_params(GBInterpreter::jp_conditional, core,
                               opcode >> 3 & 0b11);
      jump_emitted = true;

    } else if (opcode == 0b1110'0010) {
      emit_fallback_no_params(GBInterpreter::ld_c_a, core);

    } else if (opcode == 0b1111'1010) {
      emit_fallback_no_params(GBInterpreter::ld_a_u16, core);
      dyn_pc += 2;

    } else if (opcode == 0b1111'0010) {
      emit_fallback_no_params(GBInterpreter::ld_a_c, core);

    } else if (opcode == 0b1100'0011) {
      emit_fallback_no_params(GBInterpreter::jp_u16, core);
      jump_emitted = true;
      dyn_pc += 2;

    } else if (opcode == 0b1111'0011) {
      emit_fallback_no_params(GBInterpreter::di, core);

    } else if (opcode == 0b1111'1011) {
      emit_fallback_no_params(GBInterpreter::ei, core);

    } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0b0100) {
      emit_fallback_one_params(GBInterpreter::call_conditional, core,
                               opcode >> 3 & 0b11);
      jump_emitted = true;

    } else if (opcode == 0xCB) {
      const auto second = core.mem_read<uint8_t>(dyn_pc++);
      static_cycles_taken += extended_instr_timing[second] * 4;
      if (second >> 3 == 0b00111) {
        emit_fallback_one_params(GBInterpreter::srl, core, second & 0x7);

      } else if (second >> 3 == 0b00011) {
        emit_fallback_one_params(GBInterpreter::rr, core, second & 0x7);

      } else if (second >> 3 == 0b00110) {
        emit_fallback_one_params(GBInterpreter::swap, core, second & 0x7);

      } else if (second >> 3 == 0b00000) {
        emit_fallback_one_params(GBInterpreter::rlc, core, second & 0x7);

      } else if (second >> 3 == 0b00001) {
        emit_fallback_one_params(GBInterpreter::rrc, core, second & 0x7);

      } else if (second >> 3 == 0b00010) {
        emit_fallback_one_params(GBInterpreter::rl, core, second & 0x7);

      } else if (second >> 3 == 0b00100) {
        emit_fallback_one_params(GBInterpreter::sla, core, second & 0x7);

      } else if (second >> 3 == 0b00101) {
        emit_fallback_one_params(GBInterpreter::sra, core, second & 0x7);

      } else if (second >> 6 == 0b01) {
        emit_fallback_two_params(GBInterpreter::bit, core, second & 0x7,
                                 second >> 3 & 0x7);

      } else if (second >> 6 == 0b10) {
        emit_fallback_two_params(GBInterpreter::res, core, second & 0x7,
                                 second >> 3 & 0x7);

      } else if (second >> 6 == 0b11) {
        emit_fallback_two_params(GBInterpreter::set, core, second & 0x7,
                                 second >> 3 & 0x7);

      } else {
        PANIC("Unhandled bit opcode: 0x{:02X} | 0b{:08b}\n", second, second);
      }

    } else if (opcode >> 6 == 0b11 && (opcode & 0xf) == 0b0101) {
      emit_fallback_one_params(GBInterpreter::push_r16, core,
                               opcode >> 4 & 0b11);

    } else if (opcode == 0b1100'1101) {
      emit_fallback_no_params(GBInterpreter::call_u16, core);
      dyn_pc += 2;

    } else if (opcode == 0b1111'1110) {
      emit_fallback_one_params(GBInterpreter::cp_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));

    } else if (opcode == 0b1110'0110) {
      emit_fallback_one_params(GBInterpreter::and_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));

    } else if (opcode == 0b1100'0110) {
      emit_fallback_one_params(GBInterpreter::add_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));

    } else if (opcode == 0b1101'0110) {
      emit_fallback_one_params(GBInterpreter::sub_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));

    } else if (opcode == 0b1110'1110) {
      emit_fallback_one_params(GBInterpreter::xor_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));

    } else if (opcode == 0b1100'1110) {
      emit_fallback_one_params(GBInterpreter::addc_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));

    } else if (opcode == 0b1111'0110) {
      emit_fallback_one_params(GBInterpreter::or_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));

    } else if (opcode == 0b1101'1110) {
      emit_fallback_one_params(GBInterpreter::subc_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));

    } else if (opcode >> 6 == 0b11 && (opcode & 0x7) == 0b111) {
      emit_fallback_one_params(GBInterpreter::rst, core, opcode >> 3 & 0x7);
      jump_emitted = true;

    } else {
      PANIC("Unhandled opcode: 0x{:02X} | 0b{:08b}\n", opcode, opcode);
    }

    code.add(SAVED1, eax);

    // reasons to exit a block:
    // -> the page boundary has been reached
    // -> any instruction that may modify the pc has been emitted
    bool old_page = initial_dyn_pc >> PAGE_SHIFT;
    bool new_page = dyn_pc >> PAGE_SHIFT;
    if (old_page != new_page || jump_emitted ||
        (dyn_pc & (PAGE_SIZE - 1)) == 0) {
      core.pc = dyn_pc;
      break;
    }
  }

  code.mov(eax, SAVED1);
  code.add(eax, static_cycles_taken);
  emit_epilogue(core);

  return emitted_function;
}

int GBCachedInterpreter::decode_execute(Core& core) {
  auto& page = block_page_table[core.pc >> PAGE_SHIFT];
  if (!page) {
    page = new block_fp[PAGE_SIZE]();
  }

  auto& block = page[core.pc & (PAGE_SIZE - 1)];
  if (!block) {
    block = recompile_block(core);
  }

  auto cycles_taken = (*block)();
  return cycles_taken;
}
