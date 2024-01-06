#pragma once

#include "common.h"
#include "config.h"
#include <array>
#include <cstdint>

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
    explicit PPU(Core& core) : core(core) {
      // TODO: lcdstat
    }

    enum class PPUMode { OAMScan, DrawingPixels, HBlank, VBlank };

    PPUMode mode = Core::PPU::PPUMode::OAMScan;
    int dot_clock = 0;
    std::array<uint32_t, 4> colors = {0xe0f8d0, 0x88c070, 0x346856, 0x081820};

    void tick(int cycles);
    int WLC = 0;

    void draw_bg();
    void draw_sprites();
    void draw_scanline();
    void do_lyc_check();
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
  int handle_interrupts();

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
  uint8_t& handle_mmio(uint32_t addr, bool write);

  // cartridge functions
  void load_bootrom(const char* path);
  void load_rom(const char* path);

  // mmio
  uint8_t LCDC;
  uint8_t STAT;
  uint8_t SB;
  uint8_t IF;
  uint8_t IE;
  uint8_t LY;
  uint8_t LYC;
  uint8_t SCX;
  uint8_t SCY;
  uint8_t WX;
  uint8_t WY;
  uint8_t BGP;
  uint8_t OBP0;
  uint8_t OBP1;
  uint8_t STUB;

public:
  Core(Config config);
  void run_frame();
  [[nodiscard]] const auto& get_fb_ref() const { return fb; }
};
