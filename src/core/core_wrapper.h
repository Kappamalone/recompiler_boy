#pragma once

#include "common.h"
#include "core.h"
#include <chrono>
#include <functional>
#include <numeric>
#include <utility>

// Wraps the code needed to connect an emulator core to the frontend and provide
// diagnostics like eg: frame time
class CoreWrapper {
private:
  Core core;
  std::chrono::nanoseconds frame_time;

  std::function<void()> wait_for_ui_thread;
  std::function<void()> ping_ui_thread;

public:
  CoreWrapper(std::function<void()> start, std::function<void()> end)
      : core("", "../roms/gb-test-roms/cpu_instrs/individual/06-ld r,r.gb"),
        wait_for_ui_thread(std::move(start)), ping_ui_thread(std::move(end)) {}

  void run() {
    using frame_period = std::chrono::duration<long long, std::ratio<1, 60>>;
    auto end_time = std::chrono::high_resolution_clock::now();
    auto start_time = end_time;
    frame_time = end_time - start_time;

    while (true) {
      wait_for_ui_thread();

      start_time = std::chrono::high_resolution_clock::now();
      core.run_frame();
      end_time = std::chrono::high_resolution_clock::now();
      frame_time = end_time - start_time;
      double frame_time_d = std::chrono::duration<double>{frame_time}.count();

      ping_ui_thread();
    }
  }

  [[nodiscard]] const auto& get_fb_ref() const { return core.get_fb_ref(); }
  [[nodiscard]] auto get_frame_time() const { return frame_time; }
  [[nodiscard]] const Core& get_core_ref() const { return core; }
};
