#include "core.h"
#include "interpreter.h"
#include <cstdint>
#include <fstream>
#include <iostream>

static void create_logging_file(const std::string& filename) {
  std::ofstream file(filename,
                     std::ios::trunc); // trunc mode wipes the file if it exists

  if (!file.is_open()) {
    std::cerr << "Unable to create/wipe file: " << filename << std::endl;
  }
}

Core::Core(const char* bootrom_path, const char* rom_path) {
  fb.pixels.resize(fb.width * fb.height * 4);
  // logging:
  create_logging_file("../../logging.txt");

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
  wram.resize(0x2000);
  oam.resize(0xA0);
  hram.resize(0x7f);

  // mmio
  TAC = 0;
  IF = 0;
  IE = 0;
  SB = 0;

  load_rom(rom_path);
  if (strcmp(bootrom_path, "") == 0) {
    // skip bootrom initialisation
    bootrom_enabled = false;
    pc = 0x101;
    sp = 0xfffe;
    regs[0] = 0x01b0;
    regs[1] = 0x0013;
    regs[2] = 0x00d8;
    regs[3] = 0x014d;
    // TODO: lcdc and palette
  } else {
    load_bootrom(bootrom_path);
  }
}

void Core::load_rom(const char* path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    PANIC("Error opening file: {}\n", path);
  }
  file.read((char*)(bank00.data()), sizeof(uint8_t) * 0x4000);
  file.read((char*)(bank01.data()), sizeof(uint8_t) * 0x4000);
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
    return *(T*)(&bank01[addr - 0x4000]);
  } else if (in_between(0xC000, 0xDFFF, addr)) {
    return *(T*)(&wram[addr - 0xC000]);
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
    return bank01[addr - 0x4000];

  } else if (in_between(0x8000, 0x9FFF, addr)) {
    return vram[addr - 0x8000];

  } else if (in_between(0xC000, 0xDFFF, addr)) {
    return wram[addr - 0xC000];

  } else if (in_between(0xFF80, 0xFFFE, addr)) {
    return hram[addr - 0xFF80];

  } else if (in_between(0xFF00, 0xFFFF, addr)) {
    switch (addr) {
      case 0xFF01:
        return SB;
      case 0xFF02:
        printf("%c", SB);
        return STUB;
      case 0xFF07:
        return TAC;
      case 0xFF0F:
        return IF;
      case 0xFF24:
      case 0xFF25:
      case 0xFF26:
      case 0xFF40:
      case 0xFF42:
      case 0xFF43:
      case 0xFF47:
        // DPRINT("STUB: MMIO \n");
        return STUB;
      case 0xFF44:
        // DPRINT("STUB LY TO 0x90\n");
        LY = 0x90;
        return LY;
      case 0xFFFF:
        return IE;
      default:
        PANIC("Unknown memory reference at 0x{:08X}\n", addr);
    }
  } else {
    PANIC("Unknown memory reference at 0x{:08X}\n", addr);
  }
}

template <typename T>
void Core::mem_write(uint32_t addr, T value) {
  if (in_between(0x0000, 0x3FFF, addr)) {
    *(T*)(&bank00[addr]) = value;
  } else if (in_between(0xC000, 0xDFFF, addr)) {
    *(T*)(&wram[addr - 0xC000]) = value;
  } else {
    PANIC("Unknown memory write at 0x{:08X}\n", addr);
  }
}
template void Core::mem_write<uint8_t>(uint32_t addr, uint8_t value);
template void Core::mem_write<uint16_t>(uint32_t addr, uint16_t value);
template void Core::mem_write<uint32_t>(uint32_t addr, uint32_t value);

bool Core::get_flag(Regs::Flag f) {
  switch (f) {
    case Regs::Flag::Z:
      return regs[Regs::AF] >> 7 & 1;
      break;
    case Regs::Flag::N:
      return regs[Regs::AF] >> 6 & 1;
      break;
    case Regs::Flag::H:
      return regs[Regs::AF] >> 5 & 1;
      break;
    case Regs::Flag::C:
      return regs[Regs::AF] >> 4 & 1;
      break;
    default:
      PANIC("Invalid get flag");
  }
}

void Core::set_flag(Regs::Flag f, bool value) {
  switch (f) {
    case Regs::Flag::Z:
      regs[Regs::AF] = (regs[Regs::AF] & ~(1 << 7)) | (value << 7);
      break;
    case Regs::Flag::N:
      regs[Regs::AF] = (regs[Regs::AF] & ~(1 << 6)) | (value << 6);
      break;
    case Regs::Flag::H:
      regs[Regs::AF] = (regs[Regs::AF] & ~(1 << 5)) | (value << 5);
      break;
    case Regs::Flag::C:
      regs[Regs::AF] = (regs[Regs::AF] & ~(1 << 4)) | (value << 4);
      break;
    default:
      PANIC("Invalid set flag");
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
