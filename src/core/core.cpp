#include "core.h"
#include "common.h"
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

void append_to_logging(const std::string& content) {
  std::ofstream file("../logging.txt", std::ios::app);

  if (file.is_open()) {
    file << content;
    file.close();
  } else {
    std::cerr << "Unable to open file: " << std::endl;
  }
}

Core::Core(Config config) {
  fb.pixels.resize(fb.width * fb.height * 4);
  // logging:
  create_logging_file("../../logging.txt");

  // registers
  pc = 0x00;
  sp = 0x00;
  IME = false;
  req_IME = false;
  HALT = false;
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
  LCDC = 0;
  SB = 0;
  TAC = 0;
  IF = 0;
  IE = 0;
  LY = 0;
  SB = 0;
  TMA = 0;

  load_rom(config.rom_path);
  if (config.bootrom_path == nullptr) {
    // skip bootrom initialisation
    bootrom_enabled = false;
    pc = 0x100;
    sp = 0xfffe;
    regs[0] = 0x01b0;
    regs[1] = 0x0013;
    regs[2] = 0x00d8;
    regs[3] = 0x014d;
    LCDC = 0x91;
    BGP = 0xFC;
  } else {
    load_bootrom(config.bootrom_path);
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

void Core::load_bootrom(const char* path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    PANIC("Error opening file: {}\n", path);
  }
  file.read((char*)(bank00.data()), sizeof(uint8_t) * 0x100);
}

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
  } else if (in_between(0x8000, 0x9FFF, addr)) {
    return *(T*)(&vram[addr - 0x8000]);
  } else if (in_between(0xC000, 0xDFFF, addr)) {
    return *(T*)(&wram[addr - 0xC000]);
  } else if (in_between(0xFF80, 0xFFFE, addr)) {
    return *(T*)(&hram[addr - 0xFF80]);
  } else if (in_between(0xFE00, 0xFE9F, addr)) {
    return *(T*)(&oam[addr - 0xFE00]);
  } else if (in_between(0xFF00, 0xFFFF, addr)) {
    return handle_mmio<false>(addr);
  } else {
    PANIC("Unknown memory read at 0x{:08X}\n", addr);
  }
}
template uint8_t Core::mem_read<uint8_t>(uint32_t addr);
template uint16_t Core::mem_read<uint16_t>(uint32_t addr);
template uint32_t Core::mem_read<uint32_t>(uint32_t addr);

template <bool Write>
uint8_t& Core::mem_byte_reference(uint32_t addr, uint8_t value) {
  // due to me being lazy, the `write` flag controls if the reference
  // returned is uesd for modification purposes

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

  } else if (in_between(0xFE00, 0xFE9F, addr)) {
    return oam[addr - 0xFE00];

  } else if (in_between(0xFEA0, 0xFEFF, addr)) {
    return STUB;

  } else if (in_between(0xFF00, 0xFFFF, addr)) {
    return handle_mmio<Write>(addr, value);
  } else {
    PANIC("Unknown memory reference at 0x{:08X}\n", addr);
  }
}
template uint8_t& Core::mem_byte_reference<false>(uint32_t addr, uint8_t value);
template uint8_t& Core::mem_byte_reference<true>(uint32_t addr, uint8_t value);

template <typename T>
void Core::mem_write(uint32_t addr, T value) {
  if (in_between(0x0000, 0x3FFF, addr)) {
    *(T*)(&bank00[addr]) = value;
  } else if (in_between(0xC000, 0xDFFF, addr)) {
    *(T*)(&wram[addr - 0xC000]) = value;
  } else if (in_between(0xFF80, 0xFFFE, addr)) {
    *(T*)(&hram[addr - 0xFF80]) = value;
  } else if (in_between(0xFF00, 0xFFFF, addr)) {
    handle_mmio<true>(addr, value) = value;
  } else {
    PANIC("Unknown memory write at 0x{:08X}\n", addr);
  }
}
template void Core::mem_write<uint8_t>(uint32_t addr, uint8_t value);
template void Core::mem_write<uint16_t>(uint32_t addr, uint16_t value);
template void Core::mem_write<uint32_t>(uint32_t addr, uint32_t value);

template <bool Write>
uint8_t& Core::handle_mmio(uint32_t addr, uint8_t value) {
  switch (addr) {
    case 0xFF00:
      STUB = 0xff;
      return STUB;
    case 0xFF01:
      return SB;
    case 0xFF02:
      printf("%c", SB);
      return STUB;
    case 0xFF05:
      return TIMA;
    case 0xFF06:
      return TMA;
    case 0xFF07:
      return TAC;
    case 0xFF11:
    case 0xFF12:
    case 0xFF13:
    case 0xFF14:
      return STUB;
    case 0xFF50:
      // TODO: this should unload the bootrom
      return STUB;
    case 0xFF0F:
      return IF;
    case 0xFF24:
      return STUB;
    case 0xFF25:
      return STUB;
    case 0xFF26:
      return STUB;
    case 0xFF40:
      return LCDC;
    case 0xFF41:
      return STAT;
    case 0xFF42:
      return SCY;
    case 0xFF43:
      return SCX;
    case 0xFF44:
      return LY;
    case 0xFF45:
      return LYC;
    case 0xFF46:
      if constexpr (Write) {
        // DMA transfer
        // PANIC("DMA!\n", value);
      }
      return STUB;
    case 0xFF47:
      return BGP;
    case 0xFF48:
      return OBP0;
    case 0xFF49:
      return OBP1;
    case 0xFF4A:
      return WY;
    case 0xFF4B:
      return WX;
    case 0xFFFF:
      return IE;
    default:
      PRINT("Unknown memory reference at 0x{:08X}\n", addr);
      return STUB;
  }
}
template uint8_t& Core::handle_mmio<false>(uint32_t addr, uint8_t value);
template uint8_t& Core::handle_mmio<true>(uint32_t addr, uint8_t value);

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

void Core::tick_timers(int ticks) {
  static int internal_clock = 0;
  static int hz[] = {1024, 16, 64, 256};
  int select = hz[TAC & 0x3];

  for (int i = 0; i < ticks; i++) {
    internal_clock++;
    if (internal_clock % 256 == 0) {
      DIV++;
    }

    if (BIT(TAC, 2)) {
      if (internal_clock % select == 0) {
        if (TIMA == 0xFF) {
          TIMA = TMA;
          IF |= 1 << 2;
        } else {
          TIMA++;
        }
      }
    }
  }
}

int Core::handle_interrupts() {
  if (IME) {
    for (int i = 0; i < 5; i++) {
      if (BIT(IE, i) && BIT(IF, i)) {
        IME = false;
        IF &= ~(1 << i);

        static uint16_t int_vectors[] = {0x40, 0x48, 0x50, 0x58, 0x60};
        sp -= 2;
        mem_write<uint16_t>(sp, pc);

        pc = int_vectors[i];
        return 20;
      }
    }
  }

  return 0;
}

void Core::run_frame() {
  // the amount of cpu cycles to execute per frame
  // all other components are synced to this
  int cycles_to_execute = CYCLES_PER_FRAME;

  while (cycles_to_execute > 0) {
    int cycles_taken = 0;

    if (!HALT) {

      auto& core = *this;
      /*
      append_to_logging(
          fmt::format("A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: "
                      "{:02X} E: {:02X} H: {:02X} L: {:02X} SP: {:04X} PC: "
                      "00:{:04X} ({:02X} "
                      "{:02X} {:02X} {:02X})\n",
                      core.regs[Regs::AF] >> 8, core.regs[Regs::AF] & 0xff,
                      core.regs[Regs::BC] >> 8, core.regs[Regs::BC] & 0xff,
                      core.regs[Regs::DE] >> 8, core.regs[Regs::DE] & 0xff,
                      core.regs[Regs::HL] >> 8, core.regs[Regs::HL] & 0xff,
                      core.sp, core.pc, core.mem_read<uint8_t>(core.pc),
                      core.mem_read<uint8_t>(core.pc + 1),
                      core.mem_read<uint8_t>(core.pc + 2),
                      core.mem_read<uint8_t>(core.pc + 3)));
      */

      auto opcode = mem_read<uint8_t>(pc++);
      cycles_taken = GBInterpreter::decode_execute(*this, opcode);

      // enable interrupt from EI after the next instruction
      if (req_IME && opcode != 0xFB) {
        req_IME = false;
        IME = true;
      }
    } else {
      for (int i = 0; i < 5; i++) {
        if (BIT(IE, i) && BIT(IF, i)) {
          HALT = false;
        }
      }
      cycles_taken = 4;
    }
    cycles_taken += handle_interrupts();

    ppu.tick(cycles_taken);
    tick_timers(cycles_taken);

    cycles_to_execute -= cycles_taken;
  }
}
