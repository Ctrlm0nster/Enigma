/**
 * @file Input.hpp
 * @brief Input event handling, hotkey dispatching, and transient status alerts.
 */

#pragma once

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
  std::string message;      ///< Current status string.
  double expiresAt = 0;     ///< Timestamp when the notification disappears.

  /**
   * @brief Displays a status message for a default duration.
   *
   * @param value Text to display.
   */
  void show(std::string_view value);

  /**
   * @brief Retrieves the active message if not expired.
   *
   * @return String view of the active status, or empty if expired.
   */
  std::string_view current() const;
};

/**
 * @struct InterfaceState
 * @brief Holds modal state and status alert system.
 */
struct InterfaceState {
  Status status;            ///< Active status notification.
  bool photoOpen = false;   ///< Whether the full museum photo viewer is open.
};

/**
 * @brief Processes Raylib mouse and keyboard inputs for the active workbench session.
 *
 * Handles typing, backspace, clipboard copy (Ctrl+C), clipboard paste (Ctrl+V),
 * rotor adjustments, plugboard connections, and modal interactions.
 *
 * @param session Active session instance.
 * @param layout Current UI layout calculations.
 * @param state Mutable interface modal and status state.
 */
void handleInput(Session &session, const Layout &layout, InterfaceState &state);

} // namespace workbench
