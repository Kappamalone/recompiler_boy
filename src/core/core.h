#pragma once

#include "common.h"
#include "config.h"
#include <array>
#include <cstdint>

// For reference:
// The bare minimum to be able to render the most basic rom is to
// -> Load one of Blaarg's cpu tests
// -> Skip the bootrom, initialise registers are necessary
// -> Implement the instruction execution
// -> Render bg tiles

namespace Regs {
enum Regs { AF = 0, BC, DE, HL };
enum Flag { Z, N, H, C };
}; // namespace Regs

class Core {
public:
  struct {
    std::vector<uint8_t> pixels;
    size_t width = 160;
    size_t height = 144;
  } fb;

  using PPU = struct PPU {
    Core& core;
    explicit PPU(Core& core) : core(core) {}

    void draw_bg();
    void draw_scanline();
  };
  PPU ppu{*this};

  // registers
  uint16_t pc;
  uint16_t sp;
  bool IME;
  bool req_IME;
  bool HALT;
  // AF, BC, DE, HL
  std::array<uint16_t, 4> regs{};
  bool get_flag(Regs::Flag f);
  void set_flag(Regs::Flag f, bool value);

  // timers
  uint8_t DIV;
  uint8_t TIMA;
  uint8_t TMA;
  uint8_t TAC;
  void tick_timers(int ticks);

  // memory
  bool bootrom_enabled = true;
  std::vector<uint8_t> bootrom;
  std::vector<uint8_t> bank00;
  std::vector<uint8_t> bank01;
  std::vector<uint8_t> vram;
  std::vector<uint8_t> ext_ram;
  std::vector<uint8_t> wram;
  std::vector<uint8_t> oam;
  std::vector<uint8_t> hram;

  // memory read/write functions
  template <typename T>
  T mem_read(uint32_t addr);
  uint8_t& mem_byte_reference(uint32_t addr, bool write = false);
  template <typename T>
  void mem_write(uint32_t addr, T value);

  // cartridge functions
  void load_bootrom(const char* path);
  void load_rom(const char* path);

  // mmio
  uint8_t LCDC;
  uint8_t SB;
  uint8_t IF;
  uint8_t IE;
  uint8_t LY;
  uint8_t BGP;
  uint8_t STUB;

public:
  Core(Config config);
  void run_frame();
  [[nodiscard]] const auto& get_fb_ref() const { return fb; }
};
