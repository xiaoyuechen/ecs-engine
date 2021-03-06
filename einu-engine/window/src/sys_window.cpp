// Copyright (C) 2020  Xiaoyue Chen
//
// This file is part of EINU Engine.
// See <https://github.com/xiaoyuechen/einu.git>.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "einu-engine/window/sys_window.h"

#include <GLFW/glfw3.h>

#include <cassert>
#include <stdexcept>

namespace einu {
namespace window {
namespace sys {

void Init() { glfwInit(); }

void Terminate() { glfwTerminate(); }

namespace {

void SetWindowHint(const cmp::Window& window) {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, window.sample);
  if (window.mode == cmp::Window::Mode::WindowedFullScreen) {
    int count;
    auto monitors = glfwGetMonitors(&count);
    if (window.monitor_index + 1 < count) {
      throw std::runtime_error("not enough monitors");
    }
    auto monitor = monitors[window.monitor_index];
    auto mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
  }
}

GLFWmonitor* GetMonitorArgument(const cmp::Window& window) {
  switch (window.mode) {
    case cmp::Window::Mode::WindowedFullScreen:
    case cmp::Window::Mode::FullScreen: {
      int count;
      auto monitors = glfwGetMonitors(&count);
      if (window.monitor_index + 1 < count) {
        throw std::runtime_error("not enough monitors");
      }
      auto monitor = monitors[window.monitor_index];
      return monitor;
    }
    case cmp::Window::Mode::Windowed:
      return nullptr;
  }
  return nullptr;
}

auto& GetInputBuffer(GLFWwindow& window) {
  auto& win_comp =
      *static_cast<cmp::Window*>(glfwGetWindowUserPointer(&window));
  return win_comp.input_buffer;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  auto& input_buffer = GetInputBuffer(*window);
  auto&& button_state =
      input_buffer.GetMouseButton(static_cast<input::MouseButton>(button));
  button_state = action;
  auto&& mods_state = input_buffer.GetModifierKeyFlag();
  using util::operator|=;
  mods_state |= static_cast<input::ModifierKeyFlag>(mods);
}

void KeyCallback(GLFWwindow* window, int key, [[maybe_unused]] int scancode,
                 int action, int mods) {
  auto& input_buffer = GetInputBuffer(*window);
  auto key_enum = static_cast<input::KeyboardKey>(key);
  auto&& key_state = input_buffer.GetKeyboardKey(key_enum);
  key_state = action;
  auto&& mods_state = input_buffer.GetModifierKeyFlag();
  using util::operator|=;
  mods_state |= static_cast<input::ModifierKeyFlag>(mods);
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  auto& input_buffer = GetInputBuffer(*window);
  auto& cursor_pos = input_buffer.GetCursorPos();
  cursor_pos.x = static_cast<float>(xpos);
  cursor_pos.y = static_cast<float>(ypos);
}

}  // namespace

void Create(cmp::Window& win_comp) {
  SetWindowHint(win_comp);
  auto glfw_win = glfwCreateWindow(win_comp.size.width, win_comp.size.height,
                                   win_comp.title.c_str(),
                                   GetMonitorArgument(win_comp), nullptr);
  if (!glfw_win) {
    throw std::runtime_error("failed to create window " + win_comp.title);
  }

  win_comp.window = glfw_win;
  glfwSetWindowUserPointer(glfw_win, &win_comp);
  glfwSetMouseButtonCallback(glfw_win, MouseButtonCallback);
  glfwSetKeyCallback(glfw_win, KeyCallback);
  glfwSetCursorPosCallback(glfw_win, CursorPosCallback);
}

void Destroy(cmp::Window& win) { glfwDestroyWindow(win.window); }

void MakeContextCurrent(cmp::Window& win) {
  glfwMakeContextCurrent(win.window);
}

namespace {

void ResetCurrentInput(input::InputBuffer& input_buffer) noexcept {
  auto next = input_buffer.cursor + 1;
  input_buffer.cursor = next == input_buffer.kInputBufferSize ? 0 : next;

  for (auto&& buffer : input_buffer.mouse_button_buffer_table) {
    buffer[input_buffer.cursor] = false;
  }

  for (auto&& buffer : input_buffer.keyboard_key_buffer_table) {
    buffer[input_buffer.cursor] = false;
  }

  input_buffer.modifier_key_buffer[input_buffer.cursor] =
      static_cast<input::ModifierKeyFlag>(0);
}

}  // namespace

void PoolEvents(cmp::Window& win) {
  ResetCurrentInput(win.input_buffer);
  glfwPollEvents();
  win.shouldClose = glfwWindowShouldClose(win.window);
  glfwGetWindowSize(win.window, &win.size.width, &win.size.height);
  glfwGetFramebufferSize(win.window, &win.frame_size.width,
                         &win.frame_size.height);
}

void SwapBuffer(cmp::Window& win) {
  glfwSwapInterval(win.swap_interval);
  glfwSwapBuffers(win.window);
}

}  // namespace sys
}  // namespace window
}  // namespace einu
