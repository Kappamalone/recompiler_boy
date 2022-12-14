#include "core.h"

Core::Core() { fb.pixels.resize(fb.width * fb.height * 4); }

void Core::run_frame() {
  static int counter = 0;

  for (auto i = 0; i < fb.width; i++) {
    fb.pixels[i * 4 + counter * fb.width * 4] = 0xff;
    fb.pixels[i * 4 + counter * fb.width * 4 + 1] = 0xff;
    fb.pixels[i * 4 + counter * fb.width * 4 + 2] = 0xff;
    fb.pixels[i * 4 + counter * fb.width * 4 + 3] = 0xff;
  }

  counter++;
  if (counter == fb.height) {
    counter = 0;
    for (auto i = 0; i < fb.width * fb.height * 4; i++) {
      fb.pixels[i] = 0;
    }
  }
}
