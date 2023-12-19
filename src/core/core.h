#include "common.h"
#include <array>
#include <cstdint>

// For reference:
// The bare minimum to be able to render the most basic rom is to
// -> Load one of Blaarg's cpu tests
// -> Skip the bootrom, initialise registers are necessary
// -> Implement the instruction execution
// -> Render bg tiles

class Core {
public:
  struct {
    std::vector<uint8_t> pixels;
    size_t width = 160;
    size_t height = 144;
  } fb;

  // config
  bool skip_bootrom = true;

  // registers
  uint16_t pc;
  uint16_t sp;
  // AF, BC, DE, HL
  std::array<uint16_t, 4> regs{};

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
  template <typename T>
  void mem_write(uint32_t addr, T value);

  // cartridge functions
  void load_bootrom(const char* path);
  void load_rom(const char* path);

public:
  Core();
  void run_frame();
  [[nodiscard]] const auto& get_fb_ref() const { return fb; }
};
