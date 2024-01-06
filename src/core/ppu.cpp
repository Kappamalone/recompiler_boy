#include "common.h"
#include "core.h"
#include <algorithm>
#include <array>
#include <cstdint>

void Core::PPU::tick(int cycles) {
  if (!BIT(core.LCDC, 7)) {
    return;
  }

  for (int i = 0; i < cycles; i++) {
    dot_clock++;
    switch (mode) {
      case PPUMode::OAMScan:
        if (dot_clock == 80) {
          // switching to DrawingPixels
          dot_clock = 0;
          mode = PPUMode::DrawingPixels;
          core.STAT = (core.STAT & 0xFC) | 0b11;
        }
        break;
      case PPUMode::DrawingPixels:
        if (dot_clock == 172) {
          // switching to HBlank
          dot_clock = 0;
          mode = PPUMode::HBlank;
          core.STAT = (core.STAT & 0xFC) | 0b00;

          if (BIT(core.STAT, 3)) {
            core.IF |= 0b10;
          }
        }
        break;
      case PPUMode::HBlank:
        if (dot_clock == 204) {
          dot_clock = 0;
          draw_scanline();

          core.LY += 1;

          if (core.LY == 144) {
            // switch to VBlank
            mode = PPUMode::VBlank;
            core.STAT = (core.STAT & 0xFC) | 0b01;

            if (BIT(core.STAT, 4)) {
              core.IF |= 0b10;
            }
          } else {
            // switch to OAMScan
            mode = PPUMode::OAMScan;
            core.STAT = (core.STAT & 0xFC) | 0b10;
            if (BIT(core.STAT, 5)) {
              core.IF |= 0b10;
            }
          }
          do_lyc_check();
        }

        break;
      case PPUMode::VBlank:
        if (dot_clock == 456) {
          dot_clock = 0;
          core.LY += 1;

          if (core.LY == 154) {
            // switch to OAMScan
            mode = PPUMode::OAMScan;
            core.STAT = (core.STAT & 0xFC) | 0b10;
            if (BIT(core.STAT, 5)) {
              core.IF |= 0b10;
            }

            core.LY = 0;
          }
          do_lyc_check();
        }
        break;
    }
  }
}

void Core::PPU::draw_scanline() {
  draw_bg();
  draw_sprites();
}

void Core::PPU::draw_sprites() {
  if (!BIT(core.LCDC, 1)) {
    return;
  }

  int sprite_height = BIT(core.LCDC, 2) ? 16 : 8; // 8 x 16 vs 8 x 8
  int sprites_found = 0;
  typedef struct Sprite {
    int y_pos;
    int x_pos;
    int tile_num;
    uint8_t attr;
  } Sprite;
  std::array<Sprite, 10> sprites{};

  // now let's scan through OAM to get sprite data
  for (int i = 0; i < 0xA0 && sprites_found < 10; i += 4) {
    int y_pos = core.mem_read<uint8_t>(0xFE00 + i) - 16;
    int x_pos = core.mem_read<uint8_t>(0xFE00 + i + 1) - 8;
    int tile_num = core.mem_read<uint8_t>(0xFE00 + i + 2);
    auto attr = core.mem_read<uint8_t>(0xFE00 + i + 3);
    if (core.LY >= y_pos && core.LY < y_pos + sprite_height) {
      sprites[sprites_found++] = {
          .y_pos = y_pos, .x_pos = x_pos, .tile_num = tile_num, .attr = attr};
    }
  }

  std::sort(sprites.begin(), sprites.end(),
            [](const Sprite& a, const Sprite& b) { return a.x_pos < b.x_pos; });

  for (int i = 9; i >= 0; i--) {
    int y_pos = sprites[i].y_pos;
    int x_pos = sprites[i].x_pos;
    int tile_num = sprites[i].tile_num;
    auto attr = sprites[i].attr;

    bool priority = BIT(attr, 7);
    bool y_flip = BIT(attr, 6);
    bool x_flip = BIT(attr, 5);
    uint32_t palette = BIT(attr, 4) ? core.OBP1 : core.OBP0;

    // now we test to see if there is an intersection between the sprite and the
    // current scanline
    if (core.LY >= y_pos && core.LY < y_pos + sprite_height) {
      sprites_found++;
      if (x_pos < 0 || x_pos > 160) {
        return;
      }

      int row = core.LY - y_pos;
      if (y_flip) {
        row = sprite_height - row - 1;
      }

      // bit 0 of tile num should be ignored if sprite is 8x16
      if (sprite_height == 16) {
        tile_num &= ~(1);
      }
      auto target =
          core.mem_read<uint16_t>(0x8000 + (uint16_t)tile_num * 16 + row * 2);

      for (int col = 0; col < 8; col++) {
        int tile_x = x_flip ? 7 - col : col;

        uint8_t byte1 = target >> 8;
        uint8_t byte2 = target & 0xff;
        int colour_index =
            (BIT(byte2, 7 - tile_x) << 1) | (BIT(byte1, 7 - tile_x));
        uint32_t colour = colors[(palette >> (colour_index << 1)) & 0x3];
        uint32_t fb_offset = (x_pos + col + core.LY * core.fb.width) * 4;

        bool is_colour_0 =
            core.fb.pixels[fb_offset] == ((colors[0] >> 16) & 0xff) &&
            core.fb.pixels[fb_offset + 1] == ((colors[0] >> 8) & 0xff) &&
            core.fb.pixels[fb_offset + 2] == ((colors[0] >> 0) & 0xff);

        if (x_pos + col >= 160 || colour_index == 0 ||
            (priority && !is_colour_0)) {
          continue;
        }
        core.fb.pixels[fb_offset + 0] = (colour >> 16) & 0xff;
        core.fb.pixels[fb_offset + 1] = (colour >> 8) & 0xff;
        core.fb.pixels[fb_offset + 2] = (colour >> 0) & 0xff;
        core.fb.pixels[fb_offset + 3] = 0xff;
      }
    }
  }
}

void Core::PPU::draw_bg() {
  // tilemap : maps tile data into a scene
  // tiledata: raw pixel data, colors index into a palette

  uint16_t tilemap_start = 0;
  const uint16_t tiledata_start = BIT(core.LCDC, 4) ? 0x8000 : 0x9000;
  const bool signed_addressing = !BIT(core.LCDC, 4);

  for (uint8_t x = 0; x < 160; x++) {
    uint8_t xcoord_offset = 0;
    uint8_t ycoord_offset = 0;

    // window variables
    bool using_window = false;
    uint16_t window_x = (int16_t)(int8_t)core.WX - (uint16_t)7;
    uint16_t window_y = core.WY;
    if (BIT(core.LCDC, 5) && window_y <= core.LY) {
      using_window = true;
    }

    if (using_window && window_x <= x) {
      // rendering window
      xcoord_offset = x - window_x;
      ycoord_offset = core.LY - window_y;
      tilemap_start = BIT(core.LCDC, 6) ? 0x9C00 : 0x9800;
    } else {
      // rendering background
      xcoord_offset = x + core.SCX;
      ycoord_offset = core.LY + core.SCY;
      tilemap_start = BIT(core.LCDC, 3) ? 0x9C00 : 0x9800;
    }

    int row = ycoord_offset % 8;
    int tilemap_offset = (ycoord_offset / 8) * 32;

    int tile = xcoord_offset / 8;
    int col = xcoord_offset % 8;

    auto tile_num =
        core.mem_read<uint8_t>(tilemap_start + tilemap_offset + tile);

    uint16_t target = 0;
    if (signed_addressing) {
      target = (row * 2) + ((int16_t)(int8_t)tile_num * 16) + tiledata_start;
    } else {
      target = (row * 2) + ((uint16_t)tile_num * 16) + tiledata_start;
    }

    auto byte1 = core.mem_read<uint8_t>(target);
    auto byte2 = core.mem_read<uint8_t>(target + 1);

    // the color id is formed by vertically aligning byte 1 and 2
    // we then index to the bgp using that id to get the color

    int colourIndex = (BIT(byte2, 7 - col) << 1) | (BIT(byte1, 7 - col));
    if (!BIT(core.LCDC, 0)) {
      colourIndex = 0;
    }
    uint32_t colour = colors[(core.BGP >> (colourIndex << 1)) & 0x3];
    core.fb.pixels[(x + core.LY * core.fb.width) * 4 + 0] =
        (colour >> 16) & 0xff;
    core.fb.pixels[(x + core.LY * core.fb.width) * 4 + 1] =
        (colour >> 8) & 0xff;
    core.fb.pixels[(x + core.LY * core.fb.width) * 4 + 2] =
        (colour >> 0) & 0xff;
    core.fb.pixels[(x + core.LY * core.fb.width) * 4 + 3] = 0xff;
  }
}

void Core::PPU::do_lyc_check() {
  if (core.LY == core.LYC) {
    core.STAT |= (1 << 2);

    if (BIT(core.STAT, 6)) {
      core.IF |= 0b10;
    }

  } else {
    core.STAT &= ~(1 << 2);
  }
}
