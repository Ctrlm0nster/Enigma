/**
 * @file Input.hpp
 * @brief Input event handling, hotkey dispatching, and transient status alerts.
 */

#pragma once

#include "Geometry.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <string_view>

namespace workbench {

class Session;
struct Layout;

/**
 * @struct Status
 * @brief Manages transient on-screen notification messages with expiration timers.
 */
struct Status {
  std::string message;
  std::chrono::steady_clock::time_point expiresAt{};

  void show(std::string_view value);
  std::string_view current() const;
  bool isExpired() const;
  double secondsRemaining() const;
};

/**
 * @struct InterfaceState
 * @brief Holds modal state and status alert system.
 */
struct InterfaceState {
  Status status;
  bool photoOpen = false;
};

enum class EventType {
  Key,
  Char,
  MouseButton,
  CursorPos,
  WindowResize,
  WindowClose
};

struct InputEvent {
  EventType type{EventType::Key};
  int key{0};
  int scancode{0};
  int action{0}; // 0 = release, 1 = press, 2 = repeat
  int mods{0};   // 1 = shift, 2 = control, 4 = alt, 8 = super
  uint32_t codepoint{0};
  int button{0}; // 0 = left, 1 = right, 2 = middle
  Vector2 cursor{};
  int width{0};
  int height{0};
};

struct PlatformContext {
  std::function<void(std::string_view)> setClipboard;
  std::function<std::string()> getClipboard;
};

/**
 * @brief Handles an ordered event and updates session/state/layout.
 * @return True if a visual change requires redrawing.
 */
bool handleEvent(Session &session, Layout &layout, InterfaceState &state,
                 const InputEvent &event, const PlatformContext &platform,
                 bool &shouldClose);

} // namespace workbench
