#include "cached_interpreter.h"

static void test() { PRINT("Hello recompiler!\n"); }

int GBCachedInterpreter::decode_execute(Core& core) {
  // let's recompile a simple hello world
  auto f = (block_fp)code.getCurr();

  code.push(rbp);
  code.mov(rbp, rsp);

  code.mov(rax, (uintptr_t)test);
  // code.mov(rcx, (uintptr_t)&core);
  // code.mov(edx, instr);
  code.call(rax);

  code.mov(rsp, rbp);
  code.pop(rbp);
  code.ret();

  f();

  PANIC("yay!\n");
}
