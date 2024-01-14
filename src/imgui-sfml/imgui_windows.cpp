#include "core_wrapper.h"
#include "gui.h"

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

  /*

  static std::vector<uint8_t> tilemap_fb;
  static bool first_pass = true;
  if (first_pass) {
    // 384 tiles of 8 by 8 size, with 4 bytes per pixel
    tilemap_fb.resize(384 * 8 * 8 * 4);
    first_pass = false;
  }

  ImGui::Begin("Tilemap", &p_open, window_flags);
  // we have to render the tilemap in order to actually see it, so let's do that
  for (int i = 0; i < 1000 * 4; i += 4) {
    tilemap_fb[i * 4] = 0x55;
    tilemap_fb[(i + 1) * 4] = 0x55;
    tilemap_fb[(i + 2) * 4] = 0x55;
    tilemap_fb[(i + 3) * 4] = 0x55;
  }

  ImGui::Image((void*)tilemap_fb.data(), ImVec2(100, 100));

  ImGui::End();
  */
}

void Frontend::draw_imgui_windows() {
  static sf::Clock deltaClock;
  ImGui::SFML::Update(window, deltaClock.restart());

  // draw_frame_counter(core);
  draw_tile_map(core);
}
