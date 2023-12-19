#include "interpreter.h"

Core::Core() {
  fb.pixels.resize(fb.width * fb.height * 4);

  // registers
  pc = 0x00;
  sp = 0x00;
  regs.fill(0);

  // memory
  bootrom.resize(0x100);
  bank00.resize(0x4000);
  bank01.resize(0x4000);
  vram.resize(0x2000);
  ext_ram.resize(0x2000);
  wram.resize(0x1000);
  oam.resize(0xA0);
  hram.resize(0x7f);

  if (skip_bootrom) {

  } else {
  }
}

void Core::run_frame() {
  static int counter = 0;

  GBInterpreter::execute_func(*this);

  for (auto i = 0; i < fb.width; i++) {
    fb.pixels[i * 4 + counter * fb.width * 4] = 0xff;
    fb.pixels[i * 4 + counter * fb.width * 4 + 1] = 0xff;
    fb.pixels[i * 4 + counter * fb.width * 4 + 2] = 0xff;
    fb.pixels[i * 4 + counter * fb.width * 4 + 3] = 0xff;
  }

  counter++;
  if (counter == fb.height) {
    counter = 0;
    for (auto i = 0; i < fb.width * fb.height * 4; i++) {
      fb.pixels[i] = 0;
    }
  }
}
