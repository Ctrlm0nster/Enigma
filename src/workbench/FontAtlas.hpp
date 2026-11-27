/**
 * @file FontAtlas.hpp
 * @brief CPU font rasterization using FreeType and glyph atlas generation.
 */

#pragma once

#include "Geometry.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace workbench {

inline constexpr std::array<uint32_t, 10> controlGlyphs{
    0x00B7, // ·
    0x2013, // –
    0x2026, // …
    0x2192, // →
    0x2212, // −
    0x25BE, // ▾
    0x25BC, // ▼
    0x2014, // —
    0x03B2, // β
    0x03B3  // γ
};

/**
 * @brief Decodes the next UTF-8 codepoint from the string.
 */
inline uint32_t decodeUtf8(std::string_view str, std::size_t &i) {
  if (i >= str.size()) {
    return 0;
  }
  const auto byteAt = [str](std::size_t offset) -> uint32_t {
    return static_cast<unsigned char>(str[offset]);
  };
  const auto c0 = byteAt(i);
  if ((c0 & 0x80U) == 0) {
    ++i;
    return c0;
  }
  if ((c0 & 0xE0U) == 0xC0U && i + 1 < str.size()) {
    const auto c1 = byteAt(i + 1);
    i += 2;
    return ((c0 & 0x1FU) << 6U) | (c1 & 0x3FU);
  }
  if ((c0 & 0xF0U) == 0xE0U && i + 2 < str.size()) {
    const auto c1 = byteAt(i + 1);
    const auto c2 = byteAt(i + 2);
    i += 3;
    return ((c0 & 0x0FU) << 12U) | ((c1 & 0x3FU) << 6U) | (c2 & 0x3FU);
  }
  if ((c0 & 0xF8U) == 0xF0U && i + 3 < str.size()) {
    const auto c1 = byteAt(i + 1);
    const auto c2 = byteAt(i + 2);
    const auto c3 = byteAt(i + 3);
    i += 4;
    return ((c0 & 0x07U) << 18U) | ((c1 & 0x3FU) << 12U) |
           ((c2 & 0x3FU) << 6U) | (c3 & 0x3FU);
  }
  ++i;
  return '?';
}

struct GlyphMetrics {
  uint32_t codepoint{0};
  float advanceX{0.0F};
  float bearingX{0.0F};
  float bearingY{0.0F};
  float width{0.0F};
  float height{0.0F};
  float u0{0.0F};
  float v0{0.0F};
  float u1{0.0F};
  float v1{0.0F};
};

class FontAtlas {
public:
  FontAtlas();
  ~FontAtlas();

  FontAtlas(const FontAtlas &) = delete;
  FontAtlas &operator=(const FontAtlas &) = delete;
  FontAtlas(FontAtlas &&) noexcept;
  FontAtlas &operator=(FontAtlas &&) noexcept;

  bool load(const std::string &path, int basePixelSize,
            std::span<const uint32_t> extraCodepoints = {});

  Vector2 measure(std::string_view text, float fontSize,
                  float spacing = 0.0F) const;
  float baselineOffset(float fontSize) const;
  float lineAdvance(float fontSize) const;
  float centeredLineTop(float centerY, float fontSize) const;

  const GlyphMetrics *findGlyph(uint32_t codepoint) const;

  int baseSize() const { return baseSize_; }
  float ascent() const { return ascent_; }
  float descent() const { return descent_; }
  float lineHeight() const { return lineHeight_; }
  float capHeight() const { return capHeight_; }
  int atlasWidth() const { return atlasWidth_; }
  int atlasHeight() const { return atlasHeight_; }
  const std::vector<uint8_t> &bitmap() const { return bitmap_; }

  uint32_t textureId() const { return textureId_; }
  void setTextureId(uint32_t id) { textureId_ = id; }

private:
  int baseSize_{0};
  float ascent_{0.0F};
  float descent_{0.0F};
  float lineHeight_{0.0F};
  float capHeight_{0.0F};
  int atlasWidth_{1024};
  int atlasHeight_{1024};
  std::vector<uint8_t> bitmap_;
  std::unordered_map<uint32_t, GlyphMetrics> glyphs_;
  uint32_t textureId_{0};
};

} // namespace workbench
