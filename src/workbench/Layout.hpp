/**
 * @file Layout.hpp
 * @brief Dynamic layout geometry calculation for Enigma Workbench UI.
 *
 * Computes bounding boxes, widget geometry, button hitboxes, keyboard button locations,
 * and photo preview bounds across responsive window dimensions.
 */

#pragma once

#include "Geometry.hpp"
#include "Session.hpp"

#include <array>

namespace workbench {

inline constexpr int minimumWidth = 1080;   ///< Minimum supported window width.
inline constexpr int minimumHeight = 720;   ///< Minimum supported window height.

/**
 * @struct Stepper
 * @brief Represents a paired decrement/increment control hitbox.
 */
struct Stepper {
  Rectangle decrease; ///< Decrement button bounds (-).
  Rectangle increase; ///< Increment button bounds (+).
};

/**
 * @struct RotorRow
 * @brief Layout geometry for a single rotor row in the control panel.
 */
struct RotorRow {
  Rectangle bounds;      ///< Total bounding box of the rotor row.
  Stepper start;         ///< Starting position adjustment stepper.
  Stepper ring;          ///< Ringstellung adjustment stepper.
  Rectangle liveWindow;  ///< Currently visible live letter window.
};

/**
 * @struct Layout
 * @brief Comprehensive UI coordinate and bounding box hierarchy.
 */
struct Layout {
  int screenWidth{0};       ///< Stored window pixel width.
  int screenHeight{0};      ///< Stored window pixel height.
  Rectangle content;        ///< Centered main content area.
  Rectangle message;        ///< Tape display card.
  Rectangle signal;         ///< Electrical signal trace card.
  Rectangle keyboard;       ///< Keyboard and lampboard card.
  Rectangle machine;        ///< Settings control panel card.
  Rectangle photo;          ///< Museum photo preview box.
  Rectangle openPhoto;      ///< Open photo modal button.
  Rectangle photoViewer;    ///< Full photo modal window.
  Rectangle closePhoto;     ///< Close photo modal button.
  Rectangle paste;          ///< Paste clipboard button.
  Rectangle copy;           ///< Copy ciphertext button.
  Rectangle clear;          ///< Clear tape button.
  Rectangle reset;          ///< Reset machine button.
  Rectangle clearPlugboard; ///< Clear all plugboard leads button.
  Stepper rotorCountControl; ///< Add or remove the rightmost rotor.

  std::array<RotorRow, maximumRotorCount> rotorRows; ///< Active rotor rows come first.
  std::array<Rectangle, 26> keys;             ///< Bounds for 26 keyboard keys.
  std::array<Vector2, 26> plugCenters;        ///< Center coordinates for 26 plugboard sockets.

  float plugRadius;     ///< Radius for plugboard socket circles.
  float rotorsTop;      ///< Y offset for rotors section.
  float plugboardTop;   ///< Y offset for plugboard section.
};

/**
 * @brief Computes a complete UI layout given target window dimensions.
 *
 * @param screenWidth Current window pixel width.
 * @param screenHeight Current window pixel height.
 * @param rotorCount Number of active rotor rows.
 * @return Computed Layout structure.
 */
Layout makeLayout(int screenWidth, int screenHeight, std::size_t rotorCount);

} // namespace workbench
