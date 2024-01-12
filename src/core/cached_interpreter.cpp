#include "cached_interpreter.h"
#include "common_recompiler.h"
#include "interpreter.h"

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

block_fp GBCachedInterpreter::recompile_block(Core& core) {
  check_emitted_cache();

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
    if (opcode != 0xCB) {
      static_cycles_taken += regular_instr_timing[opcode] * 4;
    }

    if (opcode == 0x00) {
      // do nothing

    } else if (opcode == 0xC3) {
      emit_fallback_no_params(GBInterpreter::jp_u16, core);
      jump_emitted = true;

    } else if (opcode == 0xCB) {
      const auto second = core.mem_read<uint8_t>(core.pc++);
      static_cycles_taken += extended_instr_timing[second] * 4;
      PANIC("Unhandled bit opcode: 0x{:02X} | 0b{:08b}\n", second, second);
    } else {
      PANIC("Unhandled opcode: 0x{:02X} | 0b{:08b}\n", opcode, opcode);
    }

    code.add(SAVED1, eax);

    // reasons to exit a block:
    // -> the page boundary has been reached
    // -> any instruction that may modify the pc has been emitted
    if ((dyn_pc & (PAGE_SIZE - 1)) == 0 || jump_emitted) {
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
