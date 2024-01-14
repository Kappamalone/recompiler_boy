#include "core_wrapper.h"
#include "gui.h"
#include "imgui-SFML.h"

static void draw_frame_counter(CoreWrapper& core) {
  static double frame_time = 0;
  frame_time = std::chrono::duration<double>{core.get_frame_time()}.count();
  bool p_open = true;
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
  ImGui::SetNextWindowPos(ImVec2{10.0f, 10.0f}, ImGuiCond_Always);
  ImGui::Begin("FPS", &p_open, window_flags);
  std::string a = fmt::format("Frame time : {:.02f}ms\n", frame_time * 1000.);
  std::string b = fmt::format("FPS        : {:.02f}  \n", 1. / frame_time);
  ImGui::Text(a.c_str());
  ImGui::Text(b.c_str());
  ImGui::Separator();
  ImGui::End();
}

static void draw_tile_map(CoreWrapper& core) {
  auto& c = core.get_core_ref();
  bool p_open = true;
  ImGuiWindowFlags window_flags = 0;

  ImGui::Begin("Tilemap", &p_open, window_flags);

  static sf::Texture texture;
  static sf::Sprite sprite;
  static std::vector<uint8_t> tilemap_fb;
  static bool first_pass = true;
  if (first_pass) {
    first_pass = false;
    texture.create(128, 192);
    sprite.setTexture(texture);
    sprite.setScale(3, 3);
    tilemap_fb.resize(128 * 192 * 4);
  }

  for (int i = 0; i < 0x1800; i += 16) {
    int line_width = 128;
    int tile_coord = i / 16;

    int base_row = (tile_coord / (line_width / 8)) * 8;
    int base_col = (tile_coord % (line_width / 8)) * 8;

    for (int row = 0; row < 8; row++) {
      uint8_t byte1 = c.vram[i + row * 2];
      uint8_t byte2 = c.vram[i + row * 2 + 1];
      for (int col = 0; col < 8; col++) {
        int colour_index = (BIT(byte2, 7 - col) << 1) | (BIT(byte1, 7 - col));
        uint32_t colour = c.ppu.colors[(c.BGP >> (colour_index << 1)) & 0x3];

        uint32_t fb_offset =
            (base_col + col + (base_row + row) * line_width) * 4;
        tilemap_fb[fb_offset + 0] = (colour >> 16) & 0xff;
        tilemap_fb[fb_offset + 1] = (colour >> 8) & 0xff;
        tilemap_fb[fb_offset + 2] = (colour >> 0) & 0xff;
        tilemap_fb[fb_offset + 3] = 0xff;
      }
    }
  }

  /*
  for (int i = 0; i < 128; i++) {
    for (int j = 0; j < 192; j++) {
      tilemap_fb[(i + j * 128) * 4] = 0xff;
      tilemap_fb[(i + j * 128) * 4 + 1] = 0xff;
      tilemap_fb[(i + j * 128) * 4 + 2] = 0xff;
      tilemap_fb[(i + j * 128) * 4 + 3] = 0xff;
    }
  }
  */

  texture.update(tilemap_fb.data());
  ImGui::Image(sprite);

  ImGui::End();
}

void Frontend::draw_imgui_windows() {
  static sf::Clock deltaClock;
  ImGui::SFML::Update(window, deltaClock.restart());

  draw_frame_counter(core);
  draw_tile_map(core);
}
