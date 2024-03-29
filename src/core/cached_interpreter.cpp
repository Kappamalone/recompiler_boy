#include "cached_interpreter.h"
#include "common_recompiler.h"
#include "interpreter.h"
#include <cstdint>

void GBCachedInterpreter::emit_prologue(Core& core) {
  code.push(rbp);
  code.mov(rbp, rsp);

  code.push(SAVED1);
  code.push(SAVED2);
  code.sub(rsp, 8);
}

void GBCachedInterpreter::emit_epilogue(Core& core) {
  code.add(rsp, 8);
  code.pop(SAVED2);
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
  code.mov(PARAM2, (int64_t)first);
  code.call(rax);
}

void GBCachedInterpreter::emit_fallback_two_params(two_params_fp fallback,
                                                   Core& core, int first,
                                                   int second) {
  code.mov(rax, (uintptr_t)fallback);
  code.mov(PARAM1, (uintptr_t)&core);
  code.mov(PARAM2, (int64_t)first);
  code.mov(PARAM3, (int64_t)second);
  code.call(rax);
}

block_fp GBCachedInterpreter::recompile_block(Core& core) {
  check_emitted_cache();

  auto emitted_function = (block_fp)code.getCurr();
  auto dyn_pc = core.pc;
  auto static_cycles_taken = 0;
  bool jump_emitted = false;

  emit_prologue(core);
  // SAVED1 will hold all dynamically emitted cycles
  code.mov(SAVED1, 0);
  // SAVED2 will hold a pointer to the core
  code.mov(SAVED2, (uintptr_t)&core);

  // For the functions we call, rax holds the additional cycles to add onto
  // the static cycles we calculate. However, for certain functions (eg NOP)
  // rax is not set to anything, which can cause problems!!
  code.mov(rax, 0);

  // At compile time, we know what static cycles to add onto the
  // PC. However, we still have to account for conditional cycles

  while (true) {
    // PRINT("DYN PC: 0x{:04X}\n", dyn_pc);
    const auto initial_dyn_pc = dyn_pc;
    const auto opcode = core.mem_read<uint8_t>(dyn_pc++);
    code.add(dword[SAVED2 + get_offset(core, &core.pc)], 1);

    if (opcode != 0xCB) {
      static_cycles_taken += regular_instr_timing[opcode] * 4;
    }

    if (opcode == 0x00) {

    } else if (opcode == 0x10) {
      code.add(dword[SAVED2 + get_offset(core, &core.pc)], 1);
      dyn_pc += 1;

    } else if (opcode == 0b0000'1000) {
      // PANIC("42!\n");
      emit_fallback_no_params(GBInterpreter::ld_u16_sp, core);
      dyn_pc += 2;

    } else if (opcode == 0b0001'1000) {
      // PANIC("41!\n");
      emit_fallback_no_params(GBInterpreter::jr_unconditional, core);
      jump_emitted = true;

    } else if (opcode == 0b1110'1010) {
      // PANIC("40!\n");
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
      // PANIC("39!\n");
      emit_fallback_one_params(GBInterpreter::add_hl_r16, core,
                               opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0010) {
      emit_fallback_one_params(GBInterpreter::ld_r16_a_addr, core,
                               opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1010) {
      emit_fallback_one_params(GBInterpreter::ld_a_r16_addr, core,
                               opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b0011) {
      // PANIC("38!\n");
      emit_fallback_one_params(GBInterpreter::inc_r16, core,
                               opcode >> 4 & 0b11);

    } else if ((opcode & 0xC0) == 0 && (opcode & 0x0f) == 0b1011) {
      // PANIC("37!\n");
      emit_fallback_one_params(GBInterpreter::dec_r16, core,
                               opcode >> 4 & 0b11);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b100) {
      emit_fallback_one_params(GBInterpreter::inc_r8, core, opcode >> 3 & 0x7);

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b101) {
      emit_fallback_one_params(GBInterpreter::dec_r8, core, opcode >> 3 & 0x7);

    } else if (opcode == 0b0111'0110) {
      // PANIC("how to handle halt?");
      emit_fallback_no_params(GBInterpreter::halt, core);
      jump_emitted = true; // immediately exit, in order to turn cpu core off

    } else if (opcode >> 6 == 0b00 && (opcode & 0x7) == 0b110) {
      emit_fallback_one_params(GBInterpreter::ld_r8_u8, core,
                               opcode >> 3 & 0x7);
      dyn_pc++;

    } else if (opcode == 0b0010'0111) {
      // PANIC("36!\n");
      emit_fallback_no_params(GBInterpreter::daa, core);

    } else if (opcode == 0b0001'1111) {
      // PANIC("35!\n");
      emit_fallback_no_params(GBInterpreter::rra, core);

    } else if (opcode == 0b0010'1111) {
      // PANIC("34!\n");
      emit_fallback_no_params(GBInterpreter::cpl, core);

    } else if (opcode == 0b0011'0111) {
      // PANIC("33!\n");
      emit_fallback_no_params(GBInterpreter::scf, core);

    } else if (opcode == 0b0011'1111) {
      // PANIC("32!\n");
      emit_fallback_no_params(GBInterpreter::ccf, core);

    } else if (opcode == 0b0000'0111) {
      // PANIC("31!\n");
      emit_fallback_no_params(GBInterpreter::rlca, core);

    } else if (opcode == 0b0001'0111) {
      // PANIC("30!\n");
      emit_fallback_no_params(GBInterpreter::rla_acc, core);

    } else if (opcode == 0b0000'1111) {
      // PANIC("29!\n");
      emit_fallback_no_params(GBInterpreter::rrca, core);

    } else if (opcode >> 6 == 0b01) {
      emit_fallback_two_params(GBInterpreter::ld_r8_r8, core, opcode >> 3 & 0x7,
                               opcode & 0x7);

    } else if (opcode >> 3 == 0b10110) {
      // FIXME: get_r8 will not work because we are in compile time land!!
      emit_fallback_one_params(GBInterpreter::or_a_r8, core, opcode & 0x7);

    } else if (opcode >> 3 == 0b10101) {
      // PANIC("hit 1\n");
      emit_fallback_one_params(GBInterpreter::xor_a_r8, core, opcode & 0x7);

    } else if (opcode >> 3 == 0b10111) {
      // PANIC("hit 2\n");
      emit_fallback_one_params(GBInterpreter::cp_a_value, core, opcode & 0x7);

    } else if (opcode >> 3 == 0b10000) {
      //  PANIC("hit 3\n");
      emit_fallback_one_params(GBInterpreter::add_a_value, core, opcode & 0x7);

    } else if (opcode >> 3 == 0b10001) {
      // PANIC("hit 4\n");
      emit_fallback_one_params(GBInterpreter::addc_a_value, core, opcode & 0x7);

    } else if (opcode >> 3 == 0b10010) {
      // PANIC("hit 5\n");
      emit_fallback_one_params(GBInterpreter::sub_a_value, core, opcode & 0x7);

    } else if (opcode >> 3 == 0b10011) {
      // PANIC("hit 6\n");
      emit_fallback_one_params(GBInterpreter::subc_a_value, core, opcode & 0x7);

    } else if (opcode >> 3 == 0b10100) {
      // PANIC("hit 7\n");
      emit_fallback_one_params(GBInterpreter::and_a_value, core, opcode & 0x7);

    } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0) {
      // PANIC("28!\n");
      emit_fallback_one_params(GBInterpreter::ret_conditional, core,
                               opcode >> 3 & 0b11);
      jump_emitted = true;

    } else if (opcode == 0b1110'0000) {
      // PANIC("27!\n");
      emit_fallback_no_params(GBInterpreter::ldh_u8_a, core);
      dyn_pc++;

    } else if (opcode == 0b1110'1000) {
      // PANIC("26!\n");
      emit_fallback_no_params(GBInterpreter::add_sp_i8, core);
      dyn_pc++;

    } else if (opcode == 0b1111'0000) {
      // PANIC("25!\n");
      emit_fallback_no_params(GBInterpreter::ldh_a_u8, core);
      dyn_pc++;

    } else if (opcode == 0b1111'1000) {
      // PANIC("24!\n");
      emit_fallback_no_params(GBInterpreter::ld_hl_sp_i8, core);
      dyn_pc++;

    } else if (opcode >> 6 == 0b11 && (opcode & 0xf) == 0b0001) {
      // PANIC("23!\n");
      emit_fallback_one_params(GBInterpreter::pop_r16, core,
                               opcode >> 4 & 0b11);

    } else if (opcode == 0b1111'1001) {
      // PANIC("22!\n");
      emit_fallback_no_params(GBInterpreter::ld_sp_hl, core);

    } else if (opcode == 0b1110'1001) {
      // PANIC("21!\n");
      emit_fallback_no_params(GBInterpreter::jp_hl, core);
      jump_emitted = true;

    } else if (opcode == 0b1100'1001) {
      // PANIC("20!\n");
      emit_fallback_no_params(GBInterpreter::ret, core);
      jump_emitted = true;

    } else if (opcode == 0b1101'1001) {
      // PANIC("19!\n");
      // PANIC("interrupts? reti");
      emit_fallback_no_params(GBInterpreter::reti, core);
      jump_emitted = true;

    } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0b010) {
      // PANIC("18!\n");
      emit_fallback_one_params(GBInterpreter::jp_conditional, core,
                               opcode >> 3 & 0b11);
      jump_emitted = true;

    } else if (opcode == 0b1110'0010) {
      // PANIC("17!\n");
      emit_fallback_no_params(GBInterpreter::ld_c_a, core);

    } else if (opcode == 0b1111'1010) {
      // PANIC("16!\n");
      emit_fallback_no_params(GBInterpreter::ld_a_u16, core);
      dyn_pc += 2;

    } else if (opcode == 0b1111'0010) {
      // PANIC("15!\n");
      emit_fallback_no_params(GBInterpreter::ld_a_c, core);

    } else if (opcode == 0b1100'0011) {
      emit_fallback_no_params(GBInterpreter::jp_u16, core);
      jump_emitted = true;

    } else if (opcode == 0b1111'0011) {
      // PANIC("14!\n");
      // PANIC("interrupts? di\n");
      emit_fallback_no_params(GBInterpreter::di, core);

    } else if (opcode == 0b1111'1011) {
      // INTERRUPTS
      // PANIC("interrupts? ei\n");
      emit_fallback_no_params(GBInterpreter::ei, core);

    } else if (opcode >> 5 == 0b110 && (opcode & 0x7) == 0b0100) {
      // PANIC("13!\n");
      emit_fallback_one_params(GBInterpreter::call_conditional, core,
                               opcode >> 3 & 0b11);
      jump_emitted = true;

    } else if (opcode == 0xCB) {
      // PANIC("12!\n");
      const auto second = core.mem_read<uint8_t>(dyn_pc++);
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);
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
      // PANIC("11!\n");
      emit_fallback_one_params(GBInterpreter::push_r16, core,
                               opcode >> 4 & 0b11);

    } else if (opcode == 0b1100'1101) {
      // PANIC("10!\n");
      emit_fallback_no_params(GBInterpreter::call_u16, core);
      dyn_pc += 2;
      jump_emitted = true;

    } else if (opcode == 0b1111'1110) {
      // PANIC("9!\n");
      emit_fallback_one_params(GBInterpreter::cp_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);

    } else if (opcode == 0b1110'0110) {
      // PANIC("8!\n");
      emit_fallback_one_params(GBInterpreter::and_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);

    } else if (opcode == 0b1100'0110) {
      // PANIC("7!\n");
      emit_fallback_one_params(GBInterpreter::add_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);

    } else if (opcode == 0b1101'0110) {
      // PANIC("6!\n");
      emit_fallback_one_params(GBInterpreter::sub_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);

    } else if (opcode == 0b1110'1110) {
      // PANIC("5!\n");
      emit_fallback_one_params(GBInterpreter::xor_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);

    } else if (opcode == 0b1100'1110) {
      // PANIC("4!\n");
      emit_fallback_one_params(GBInterpreter::addc_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);

    } else if (opcode == 0b1111'0110) {
      // PANIC("3!\n");
      emit_fallback_one_params(GBInterpreter::or_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);

    } else if (opcode == 0b1101'1110) {
      // PANIC("2!\n");
      emit_fallback_one_params(GBInterpreter::subc_value, core,
                               core.mem_read<uint8_t>(dyn_pc++));
      code.add(word[SAVED2 + get_offset(core, &core.pc)], 1);

    } else if (opcode >> 6 == 0b11 && (opcode & 0x7) == 0b111) {
      // PANIC("1!\n");
      emit_fallback_one_params(GBInterpreter::rst, core, opcode >> 3 & 0x7);
      jump_emitted = true;

    } else {
      PANIC("Unhandled opcode: 0x{:02X} | 0b{:08b}\n", opcode, opcode);
    }

    code.add(SAVED1, rax);

    // reasons to exit a block:
    // -> the page boundary has been reached or crossed
    // -> any instruction that may modify the pc has been emitted
    bool old_page = initial_dyn_pc >> PAGE_SHIFT;
    bool new_page = dyn_pc >> PAGE_SHIFT;
    if (old_page != new_page || jump_emitted ||
        (dyn_pc & (PAGE_SIZE - 1)) == 0) {
      break;
    }
  }

  code.mov(rax, SAVED1);
  code.add(rax, static_cycles_taken);
  emit_epilogue(core);

  return emitted_function;
}

void GBCachedInterpreter::invalidate_page(uint16_t addr) {
  auto& page = block_page_table[addr >> PAGE_SHIFT];
  if (page) {
    delete[] page;
    page = nullptr;
  }
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
