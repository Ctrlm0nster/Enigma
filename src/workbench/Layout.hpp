#pragma once

#include "Session.hpp"
#include "raylib.h"

#include <array>

namespace workbench {

inline constexpr int minimumWidth = 1080;
inline constexpr int minimumHeight = 720;
inline constexpr float margin = 24;

struct Stepper {
  Rectangle decrease;
  Rectangle increase;
};

struct RotorRow {
  Rectangle bounds;
  Stepper start;
  Stepper ring;
  Rectangle liveWindow;
};

struct Layout {
  Rectangle message;
  Rectangle signal;
  Rectangle keyboard;
  Rectangle machine;
  Rectangle copy;
  Rectangle clear;
  Rectangle reset;
  Rectangle clearPlugboard;
  std::array<RotorRow, rotorCount> rotorRows;
  std::array<Vector2, 26> keyCenters;
  std::array<Vector2, 26> plugCenters;
  float keyRadius;
  float plugRadius;
  float plugboardTop;
};

Layout makeLayout(int screenWidth, int screenHeight);

} // namespace workbench
