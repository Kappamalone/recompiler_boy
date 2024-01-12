#include "cached_interpreter.h"
#include "common_recompiler.h"

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

block_fp GBCachedInterpreter::recompile_block(Core& core) {
  check_emitted_cache();

  auto p = (block_fp)code.getCurr();
  auto dyn_pc = core.pc;

  emit_prologue(core);
  code.mov(SAVED1, (uintptr_t)&core);

  while (true) {
    dyn_pc++;

    // if we reach a page boundary, then we must exit the block
    if ((dyn_pc & (PAGE_SIZE - 1)) == 0) {
      break;
    }
  }
  emit_epilogue(core);
  return p;
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
  PANIC("yay!\n");
  return cycles_taken;
}
