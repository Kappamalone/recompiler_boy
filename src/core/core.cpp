#include "core.h"
#include "interpreter.h"
#include <cstdint>
#include <fstream>

Core::Core(const char* bootrom_path, const char* rom_path) {
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

  load_rom(rom_path);
  if (strcmp(bootrom_path, "") == 0) {
    // skip bootrom initialisation
    bootrom_enabled = false;
    pc = 0x100;
    sp = 0xfffe;
    regs[0] = 0x01b0;
    regs[1] = 0x0013;
    regs[2] = 0x00d8;
    regs[3] = 0x014d;
    // TODO: lcdc and palette
  } else {
    load_bootrom(bootrom_path);
  }

  /*
  // simple test
  regs[Regs::BC] = 0x00ff;
  GBInterpreter::ld_r8_r8(*this, 1, 0);
  PANIC("value of BC: 0x{:04X}", regs[Regs::BC]);
  */
}

void Core::load_rom(const char* path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    PANIC("Error opening file: {}\n", path);
  }
  file.read((char*)(bank00.data()), sizeof(uint8_t) * 0x4000);
}

void Core::load_bootrom(const char* path) {}

static constexpr bool in_between(uint32_t start, uint32_t end, uint32_t addr) {
  return addr >= start && addr <= end;
}

// NOTE: Type punning is used for reading and writing. This is not portable to a
// BE host sytem

template <typename T>
T Core::mem_read(uint32_t addr) {
  if (in_between(0x0000, 0x3FFF, addr)) {
    return *(T*)(&bank00[addr]);
  } else if (in_between(0x4000, 0x7FFF, addr)) {
    return *(T*)(&bank01[addr]);
  } else if (in_between(0xC000, 0xCFFF, addr)) {
    return *(T*)(&wram[addr]);
  } else {
    PANIC("Unknown memory read at 0x{:08X}\n", addr);
  }
}
template uint8_t Core::mem_read<uint8_t>(uint32_t addr);
template uint16_t Core::mem_read<uint16_t>(uint32_t addr);
template uint32_t Core::mem_read<uint32_t>(uint32_t addr);

uint8_t& Core::mem_byte_reference(uint32_t addr) {
  if (in_between(0x0000, 0x3FFF, addr)) {
    return bank00[addr];
  } else if (in_between(0x4000, 0x7FFF, addr)) {
    return bank01[addr];
  } else if (in_between(0xC000, 0xCFFF, addr)) {
    return wram[addr];
  } else {
    PANIC("Unknown memory read at 0x{:08X}\n", addr);
  }
}

template <typename T>
void Core::mem_write(uint32_t addr, T value) {
  if (in_between(0x0000, 0x3FFF, addr)) {
    *(T*)(&bank00[addr]) = value;
  } else {
    PANIC("Unknown memory write at 0x{:08X}\n", addr);
  }
}
template void Core::mem_write<uint8_t>(uint32_t addr, uint8_t value);
template void Core::mem_write<uint16_t>(uint32_t addr, uint16_t value);
template void Core::mem_write<uint32_t>(uint32_t addr, uint32_t value);

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
