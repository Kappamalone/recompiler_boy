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
  uint16_t pc = 0;
  uint16_t sp = 0;
  bool IME = false;
  bool req_IME = false;
  bool HALT = false;
  // AF, BC, DE, HL
  std::array<uint16_t, 4> regs{};
  bool get_flag(Regs::Flag f);
  void set_flag(Regs::Flag f, bool value);

  // timers
  uint8_t DIV = 0;
  uint8_t TIMA = 0;
  uint8_t TMA = 0;
  uint8_t TAC = 0;
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
  T mem_read(uint16_t addr);
  template <bool Write = false>
  uint8_t& mem_byte_reference(uint16_t addr, uint8_t value = 0);
  template <typename T>
  void mem_write(uint16_t addr, T value);
  template <bool Write>
  uint8_t& handle_mmio(uint16_t addr, uint8_t value = 0);

  // cartridge functions
  void load_bootrom(const char* path);
  void load_rom(const char* path);

  // mmio
  uint8_t STUB = 0;
  uint8_t LCDC = 0;
  uint8_t STAT = 0;
  uint8_t SB = 0;
  uint8_t IF = 0;
  uint8_t IE = 0;
  uint8_t LY = 0;
  uint8_t LYC = 0;
  uint8_t SCX = 0;
  uint8_t SCY = 0;
  uint8_t WX = 0;
  uint8_t WY = 0;
  uint8_t BGP = 0;
  uint8_t OBP0 = 0;
  uint8_t OBP1 = 0;
  uint8_t JOYP_WRITE = 0;
  uint8_t JOYP_READ = 0;

public:
  Core(Config config, std::vector<bool>& input);
  void run_frame();
  [[nodiscard]] const auto& get_fb_ref() const { return fb; }

  std::vector<bool>& input;
  enum class Input { UP = 0, DOWN, LEFT, RIGHT, START, SELECT, A, B };
};
