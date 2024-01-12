#pragma once
#include "common_recompiler.h"
#include "core.h"

// a cached interpreter/dynamic recompiler's general flow works like this:
//
// -> Is there a block of instructions at this PC?
//  -> Yes
//    -> Run this block of instructions, tick other components by cycle count
//       of this block
//  -> No
//    -> Compile block of instructions until certain condition (see below),
//       then execute block
//
// -> Conditions to stop block compilation:
//    -> any sort of jump condition that may change the PC
//    -> block exceeds page boundary
//
// -> Conditions to invalidate blocks:
//    -> write occurs to page
//    -> MBC actions (ie, rom bank number changed)
//
//
// -> Interrupts (do they need to be serviced as soon as requested?)

using no_params_fp = int (*)(Core&);

class GBCachedInterpreter {
  inline static block_fp* block_page_table[0xffff >> PAGE_SHIFT];
  inline static x64Emitter code;

  // Get offset from a variable to the cpu core
  static uintptr_t inline get_offset(Core& core, void* variable) {
    return (uintptr_t)variable - (uintptr_t)&core;
  }

  // Check if code cache is close to being exhausted
  static void check_emitted_cache() {
    if (code.getSize() + CACHE_LEEWAY >
        CACHE_SIZE) { // We've nearly exhausted code cache, so throw it out
      code.reset();
      memset(block_page_table, 0, sizeof(block_page_table));
      PANIC("Code Cache Exhausted!!\n");
    }
  }

public:
  static void emit_prologue(Core& core);
  static void emit_epilogue(Core& core);
  static block_fp recompile_block(Core& core);
  static void emit_fallback_no_params(no_params_fp fallback, Core& core);
  static int decode_execute(Core& core);
};
