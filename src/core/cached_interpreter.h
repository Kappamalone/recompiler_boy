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

class GBCachedInterpreter {
  inline static block_fp* block_page_table[0xffff >> PAGE_SHIFT];
  inline static x64Emitter code;

  // Get offset from a variable to the cpu core
  static uintptr_t inline get_offset(Core& core, void* variable) {
    return (uintptr_t)variable - (uintptr_t)&core;
  }

public:
  static int decode_execute(Core& core);
};
