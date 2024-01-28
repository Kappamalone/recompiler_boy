#include "cached_interpreter.h"
#include "gui.h"
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>

// Function to handle Ctrl+C signal
void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "Ctrl+C received. Performing final cleanup..." << std::endl;

    // Open a file for writing in binary mode
    std::ofstream output_file("output.bin", std::ios::binary);

    // Perform your final cleanup here
    auto size = GBCachedInterpreter::code.getSize();
    GBCachedInterpreter::code.resetSize();
    output_file.write(
        reinterpret_cast<const char*>(GBCachedInterpreter::code.getCurr()),
        size);
    output_file.close();

    std::cout << "Cleanup complete. Exiting program." << std::endl;
    std::exit(EXIT_SUCCESS);
  }
}

int main(int argc, char** argv) {
  // Set up the signal handler

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
  std::signal(SIGINT, signal_handler);
  gui.run();

  return 0;
}
