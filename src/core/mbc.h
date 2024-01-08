#pragma once
#include "common.h"
#include <array>

class Core;

class MBC {
  Core& core;
  uint32_t stub = 0;

  enum class CartridgeType {
    ROM_ONLY,
    MBC1,
    MBC1_RAM,
    MBC1_RAM_BATTERY,
  };
  CartridgeType cartridge_type;
  std::array<int, 9> rom_size_map{
      32 * 1024,       64 * 1024,       128 * 1024,
      256 * 1024,      512 * 1024,      1 * 1024 * 1024,
      2 * 1024 * 1024, 4 * 1024 * 1024, 8 * 1024 * 1024};
  uint8_t rom_size;
  std::vector<uint8_t> rom;

  std::array<int, 6> ram_size_map{0,         0,          8 * 1024,
                                  32 * 1024, 128 * 1024, 64 * 1024};
  uint8_t ram_size;
  std::vector<uint8_t> ram;

  using MBC1Regs = struct MBC1Regs {
    uint8_t ram_enable = 0; // gonna ignore this
    uint8_t rom_bank_number = 0;
    uint8_t special_2_bits = 0;
    bool mode_select = false;
  };
  MBC1Regs mbc1regs;

public:
  MBC(Core& core, const char* rom_path);

  template <bool Write, typename T>
  T& mem_reference(uint16_t addr, uint8_t value = 0);
};
