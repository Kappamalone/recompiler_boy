#include "common.h"

class Core {
  struct {
    std::vector<uint8_t> pixels;
    size_t width = 160;
    size_t height = 144;
  } fb;

public:
  Core();
  void run_frame();
  [[nodiscard]] const auto& get_fb_ref() const { return fb; }
};
