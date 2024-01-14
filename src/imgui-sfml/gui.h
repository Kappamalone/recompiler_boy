#pragma once
#include "SFML/Window/Keyboard.hpp"
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
  static constexpr char WINDOW_NAME[] = "Recompiler Boy";

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
          } else if (event.key.code == sf::Keyboard::Up) {
            core.input[(int)Core::Input::UP] = true;
          } else if (event.key.code == sf::Keyboard::Down) {
            core.input[(int)Core::Input::DOWN] = true;
          } else if (event.key.code == sf::Keyboard::Left) {
            core.input[(int)Core::Input::LEFT] = true;
          } else if (event.key.code == sf::Keyboard::Right) {
            core.input[(int)Core::Input::RIGHT] = true;
          } else if (event.key.code == sf::Keyboard::A) {
            core.input[(int)Core::Input::A] = true;
          } else if (event.key.code == sf::Keyboard::S) {
            core.input[(int)Core::Input::B] = true;
          } else if (event.key.code == sf::Keyboard::D) {
            core.input[(int)Core::Input::SELECT] = true;
          } else if (event.key.code == sf::Keyboard::C) {
            core.input[(int)Core::Input::START] = true;
          }
          break;
        case sf::Event::KeyReleased:
          if (event.key.code == sf::Keyboard::Up) {
            core.input[(int)Core::Input::UP] = false;
          } else if (event.key.code == sf::Keyboard::Down) {
            core.input[(int)Core::Input::DOWN] = false;
          } else if (event.key.code == sf::Keyboard::Left) {
            core.input[(int)Core::Input::LEFT] = false;
          } else if (event.key.code == sf::Keyboard::Right) {
            core.input[(int)Core::Input::RIGHT] = false;
          } else if (event.key.code == sf::Keyboard::A) {
            core.input[(int)Core::Input::A] = false;
          } else if (event.key.code == sf::Keyboard::S) {
            core.input[(int)Core::Input::B] = false;
          } else if (event.key.code == sf::Keyboard::D) {
            core.input[(int)Core::Input::SELECT] = false;
          } else if (event.key.code == sf::Keyboard::C) {
            core.input[(int)Core::Input::START] = false;
          }
        default:
          break;
      }
    }
  }

public:
  explicit Frontend(Config config)
      : core(
            config, [this]() { return wait_for_ui_thread(); },
            [this]() { return ping_ui_thread(); }),
        window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), WINDOW_NAME) {

    core_thread = std::thread([this]() { core.run(); });

    window.setFramerateLimit(60);
    auto success = ImGui::SFML::Init(window);
    if (!success) {
      PANIC("Failed to open window!\n");
    }
    texture.create(core.get_fb_ref().width, core.get_fb_ref().height);
    sprite.setTexture(texture);
    sprite.setScale(WINDOW_WIDTH / sprite.getLocalBounds().width,
                    WINDOW_HEIGHT / sprite.getLocalBounds().height);
  }

  void run() {
    while (window.isOpen()) {
      ping_core_thread();

      // TODO: double buffering
      handle_input();
      draw_imgui_windows();
      texture.update(core.get_fb_ref().pixels.data());
      window.clear();
      window.draw(sprite);
      ImGui::SFML::Render(window);
      window.display();

      wait_for_core_thread();
    }

    exit(0);
  }

  void draw_imgui_windows();

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
