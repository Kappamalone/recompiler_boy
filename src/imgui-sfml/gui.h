#pragma once
#include "common.h"
#include "core_wrapper.h"
#include "imgui-SFML.h"
#include "imgui.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class Frontend {
private:
  static constexpr size_t WINDOW_WIDTH = 1000;
  static constexpr size_t WINDOW_HEIGHT = 1000;
  static constexpr char WINDOW_NAME[] = "TEMPLATE";

  sf::RenderWindow window;
  sf::Texture texture;
  sf::Sprite sprite;

  std::thread core_thread;
  CoreWrapper core;

  bool is_framerate_limited = true;
  void toggle_frame_limiter() { is_framerate_limited ^= true; }

  void handle_input() {
    sf::Event event{};

    while (window.pollEvent(event)) // Handle input
    {
      ImGui::SFML::ProcessEvent(window, event);
      switch (event.type) {
        case sf::Event::Closed:
          window.close();
          break;
        case sf::Event::KeyPressed:
          if (event.key.code == sf::Keyboard::F) {
            toggle_frame_limiter();
          }
          break;
        case sf::Event::KeyReleased:
          break;
        default:
          break;
      }
    }
  }

  // TODO: actually use imgui
  void draw_imgui_windows() {
    static sf::Clock deltaClock;
    ImGui::SFML::Update(window, deltaClock.restart());

    /*
    static size_t counter = 0;
    static double frame_time = 0;
    frame_time += std::chrono::duration<double>{core.get_frame_time()}.count();
    counter++;
    if (counter == 10) {
      PRINT("{:.2f} ms ", 1000 * (frame_time / 10));
      PRINT("{:.2f} fps\n", 1. / (frame_time / 10));
      counter = 0;
      frame_time = 0;
    }
    */
  }

public:
  Frontend(Config config)
      : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), WINDOW_NAME),
        core(
            config, [this]() { return wait_for_ui_thread(); },
            [this]() { return ping_ui_thread(); }) {

    core_thread = std::thread([this]() { core.run(); });

    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);
    texture.create(core.get_fb_ref().width, core.get_fb_ref().height);
    sprite.setTexture(texture);
    sprite.setScale(WINDOW_WIDTH / sprite.getLocalBounds().width,
                    WINDOW_HEIGHT / sprite.getLocalBounds().height);
  }

  void run() {
    while (window.isOpen()) {
      ping_core_thread();

      wait_for_core_thread();

      // HACK: temporarily eliminates screen tearing
      handle_input();
      draw_imgui_windows();
      texture.update(core.get_fb_ref().pixels.data());
      window.clear();
      window.draw(sprite);
      ImGui::SFML::Render(window);
      window.display();
    }

    exit(0);
  }

  // we ping the core thread, which then runs for one frame. the core
  // frame then pings back once frame execution is finished
  bool run_frame = false;
  std::mutex mutex_run_frame;
  std::condition_variable condvar_run_frame;
  void ping_core_thread() {
    // TODO: figure out why I can't place an early return here
    run_frame = true;
    condvar_run_frame.notify_one();
    // PRINT("FRAME START\n");
  }

  void wait_for_core_thread() {
    if (!this->is_framerate_limited) {
      return;
    }
    std::unique_lock<std::mutex> lock(mutex_run_frame);
    condvar_run_frame.wait(lock, [this] { return !run_frame; });
    // PRINT("FRAME END\n");
  }

  void ping_ui_thread() {
    if (!this->is_framerate_limited) {
      return;
    }
    run_frame = false;
    condvar_run_frame.notify_one();
  }

  void wait_for_ui_thread() {
    if (!this->is_framerate_limited) {
      return;
    }
    std::unique_lock<std::mutex> lock(mutex_run_frame);
    condvar_run_frame.wait(lock, [this] { return run_frame; });
  }
};
