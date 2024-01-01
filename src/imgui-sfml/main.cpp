#include "gui.h"
#include <cstring>

int main(int argc, char** argv) {
  if (argc != 2 && argc != 3) {
    PANIC("Usage: ./TEMPLATE <rom> <bootrom>?\n");
  }

  Config config{};
  config.rom_path = argv[1];
  if (argc == 3) {
    config.bootrom_path = argv[2];
  } else {
    config.bootrom_path = nullptr;
  }

  auto gui = Frontend(config);
  gui.run();
  return 0;
}
