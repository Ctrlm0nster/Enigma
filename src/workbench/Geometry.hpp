/**
 * @file Geometry.hpp
 * @brief Common 2D geometry structures, colors, and math helpers for Enigma Workbench.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace workbench {

/**
 * @struct Vector2
 * @brief 2D point or vector in single-precision floating point.
 */
struct Vector2 {
  float x{0.0F};
  float y{0.0F};

  constexpr bool operator==(const Vector2 &) const = default;
};

/**
 * @struct Rectangle
 * @brief 2D axis-aligned bounding box defined by top-left corner and size.
 */
struct Rectangle {
  float x{0.0F};
  float y{0.0F};
  float width{0.0F};
  float height{0.0F};

  constexpr bool operator==(const Rectangle &) const = default;
};

/**
 * @struct Color
 * @brief 8-bit per channel RGBA color in sRGB space.
 */
struct Color {
  uint8_t r{0};
  uint8_t g{0};
  uint8_t b{0};
  uint8_t a{255};

  constexpr bool operator==(const Color &) const = default;
};

// Workbench palette colors
inline constexpr Color background{17, 19, 22, 255};
inline constexpr Color panel{26, 29, 33, 255};
inline constexpr Color well{16, 18, 21, 255};
inline constexpr Color raised{37, 41, 47, 255};
inline constexpr Color hoverFill{50, 56, 65, 255};
inline constexpr Color border{57, 63, 72, 255};
inline constexpr Color keyEdge{82, 89, 99, 255};
inline constexpr Color shadow{11, 13, 15, 255};
inline constexpr Color foreground{247, 245, 238, 255};
inline constexpr Color muted{183, 189, 197, 255};
inline constexpr Color amber{255, 194, 109, 255};
inline constexpr Color amberHover{255, 213, 156, 255};
inline constexpr Color amberLow{64, 47, 30, 255};
inline constexpr Color amberWell{34, 28, 23, 255};
inline constexpr Color wireLit{201, 150, 83, 255};
inline constexpr Color wireIdle{110, 117, 127, 255};
inline constexpr Color white{255, 255, 255, 255};

inline constexpr float right(Rectangle box) { return box.x + box.width; }
inline constexpr float bottom(Rectangle box) { return box.y + box.height; }

inline constexpr Vector2 center(Rectangle box) {
  return {box.x + box.width * 0.5F, box.y + box.height * 0.5F};
}

inline constexpr Rectangle insetBy(Rectangle box, float amount) {
  return {box.x + amount, box.y + amount, box.width - 2.0F * amount,
          box.height - 2.0F * amount};
}

inline constexpr bool checkCollisionPointRec(Vector2 point, Rectangle rec) {
  return point.x >= rec.x && point.x <= rec.x + rec.width &&
         point.y >= rec.y && point.y <= rec.y + rec.height;
}

inline constexpr bool checkCollisionPointCircle(Vector2 point, Vector2 center,
                                                float radius) {
  const float dx = point.x - center.x;
  const float dy = point.y - center.y;
  return (dx * dx + dy * dy) <= (radius * radius);
}

inline float srgbToLinearChannel(float c) {
  return (c <= 0.04045F) ? (c / 12.92F)
                         : std::pow((c + 0.055F) / 1.055F, 2.4F);
}

inline std::array<float, 4> colorToLinear(Color c) {
  return {srgbToLinearChannel(static_cast<float>(c.r) / 255.0F),
          srgbToLinearChannel(static_cast<float>(c.g) / 255.0F),
          srgbToLinearChannel(static_cast<float>(c.b) / 255.0F),
          static_cast<float>(c.a) / 255.0F};
}

} // namespace workbench
