#include "core.h"
#include <cstdint>

void Core::PPU::draw_scanline() {
  if (!BIT(core.LCDC, 7)) {
    return;
  };

  draw_frame();
}

void Core::PPU::draw_bg() {
  if (!BIT(core.LCDC, 0)) {
    return;
  }

  // draw tile

  // tilemap : maps tile data into a scene
  // tiledata: raw pixel data, colors index into a palette

  const uint16_t tilemap_start = BIT(core.LCDC, 3) ? 0x9C00 : 0x9800;
  const uint16_t tiledata_start = BIT(core.LCDC, 4) ? 0x8000 : 0x8800;
  const bool signed_addressing = BIT(core.LCDC, 4);

  for (uint8_t y = 0; y < 144; y++) {
    for (uint8_t x = 0; x < 160; x++) {
      // bg specific
      int xcoord_offset = x + core.SCX;
      int ycoord_offset = y + core.SCY;

      int row = ycoord_offset % 8;
      int tilemap_offset = (ycoord_offset / 8) * 32;

      int tile = xcoord_offset / 8;
      int col = xcoord_offset % 8;

      auto tile_num =
          core.mem_read<uint8_t>(tilemap_start + tilemap_offset + tile);

      uint8_t byte1 = 0;
      uint8_t byte2 = 0;
      uint16_t target = 0;
      if (signed_addressing) {
        target = (row * 2) + ((int16_t)(int8_t)tile_num * (int16_t)16) +
                 tiledata_start;
      } else {
        target = (row * 2) + ((uint16_t)tile_num * 16) + tiledata_start;
      }
      byte1 = core.mem_read<uint8_t>(target);
      byte2 = core.mem_read<uint8_t>(target + 1);

      uint32_t colors[4] = {0xf6c6a8, 0xd17c7c, 0x5b768d, 0x46425e};
      int colourIndex =
          ((byte2 >> (7 - col) & 1) << 1) | (byte1 >> (7 - col) & 1);
      uint32_t colour = colors[(core.BGP >> (colourIndex * 2)) & 0x3];

      core.fb.pixels[(x + y * core.fb.width) * 4 + 0] = (colour >> 16) & 0xff;
      core.fb.pixels[(x + y * core.fb.width) * 4 + 1] = (colour >> 8) & 0xff;
      core.fb.pixels[(x + y * core.fb.width) * 4 + 2] = (colour >> 0) & 0xff;
      core.fb.pixels[(x + y * core.fb.width) * 4 + 3] = 0xff;
    }
  }
}

void Core::PPU::draw_frame() { draw_bg(); }
