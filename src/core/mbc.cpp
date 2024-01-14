#include "mbc.h"
#include "common.h"
#include "core.h"
#include <algorithm>
#include <fstream>

MBC::MBC(Core& core, const char* rom_path) : core(core) {
  // Open the file in binary mode
  std::ifstream file(rom_path, std::ios::binary);

  if (!file.is_open()) {
    PANIC("Error opening file\n");
  }

  file.seekg(0x147);
  file.read((char*)&cartridge_type, 1);
  switch ((uint8_t)cartridge_type) {
    case 0x00:
      cartridge_type = CartridgeType::ROM_ONLY;
      break;
    case 0x01:
      cartridge_type = CartridgeType::MBC1;
      break;
    case 0x03:
      cartridge_type = CartridgeType::MBC1_RAM_BATTERY;
      break;
    default:
      PANIC("Unhandled MBC of ${:02X}\n", (uint8_t)cartridge_type);
  }

  file.seekg(0x148);
  file.read((char*)&rom_size, 1);
  rom.resize(rom_size_map[rom_size]);
  PRINT("ROM SIZE: {}\n", rom_size);
  file.seekg(0);
  file.read((char*)(rom.data()), sizeof(uint8_t) * rom_size_map[rom_size]);
  ram.resize(0x2000);

  file.seekg(0x149);
  file.read((char*)&ram_size, 1);
  ram.resize(ram_size_map[ram_size]);
}

template <bool Write, typename T>
T& MBC::mem_reference(uint16_t addr, uint8_t value) {
  if constexpr (Write) {
    if (in_between(0x0000, 0x7FFF, addr)) {
      // handle MBC registers
      if (in_between(0x2000, 0x3FFF, addr)) {
        mbc1regs.rom_bank_number = value & 0x1f;
      }
    } else if (in_between(0xA000, 0xBFFF, addr)) {
      // write to external RAM

      *(T*)(&ram[addr - 0xA000]) = value;
    }

    return *(T*)&stub;
  } else {
    if (in_between(0x0000, 0x3FFF, addr)) {
      return *(T*)(&rom[addr]);

    } else if (in_between(0x4000, 0x7FFF, addr)) {
      uint32_t bank =
          (mbc1regs.rom_bank_number) == 0
              ? 1
              : mbc1regs.rom_bank_number & (~(0xff << (rom_size + 1)));
      uint32_t new_addr = (addr - 0x4000) + (bank * 0x4000);
      return *(T*)(&rom[new_addr]);

    } else if (in_between(0xA000, 0xBFFF, addr)) {
      return *(T*)(&ram[addr - 0xA000]);
    }
  }
  PANIC("unimplemented mbc!\n");
}

// explicit template instantiations
template uint8_t& MBC::mem_reference<false, uint8_t>(uint16_t addr,
                                                     uint8_t value);
template uint16_t& MBC::mem_reference<false, uint16_t>(uint16_t addr,
                                                       uint8_t value);
template uint32_t& MBC::mem_reference<false, uint32_t>(uint16_t addr,
                                                       uint8_t value);
template uint8_t& MBC::mem_reference<true, uint8_t>(uint16_t addr,
                                                    uint8_t value);
template uint16_t& MBC::mem_reference<true, uint16_t>(uint16_t addr,
                                                      uint8_t value);
template uint32_t& MBC::mem_reference<true, uint32_t>(uint16_t addr,
                                                      uint8_t value);
