#include "FontAtlas.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace workbench {

FontAtlas::FontAtlas() = default;

FontAtlas::~FontAtlas() = default;

FontAtlas::FontAtlas(FontAtlas &&) noexcept = default;
FontAtlas &FontAtlas::operator=(FontAtlas &&) noexcept = default;

bool FontAtlas::load(const std::string &path, int basePixelSize,
                     std::span<const uint32_t> extraCodepoints) {
  FT_Library ft = nullptr;
  if (FT_Init_FreeType(&ft)) {
    std::fputs("Failed to initialize FreeType\n", stderr);
    return false;
  }

  FT_Face face = nullptr;
  if (FT_New_Face(ft, path.c_str(), 0, &face)) {
    FT_Done_FreeType(ft);
    return false;
  }

  if (basePixelSize <= 0 ||
      FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(basePixelSize))) {
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return false;
  }
  baseSize_ = basePixelSize;
  ascent_ = static_cast<float>(face->size->metrics.ascender) / 64.0F;
  descent_ = static_cast<float>(face->size->metrics.descender) / 64.0F;
  lineHeight_ = static_cast<float>(face->size->metrics.height) / 64.0F;
  capHeight_ = 0.0F;

  std::vector<uint32_t> codepoints;
  codepoints.reserve(95 + extraCodepoints.size());
  for (uint32_t cp = 32; cp <= 126; ++cp) {
    codepoints.push_back(cp);
  }
  for (uint32_t cp : extraCodepoints) {
    codepoints.push_back(cp);
  }

  atlasWidth_ = 1024;
  atlasHeight_ = 1024;
  bitmap_.assign(static_cast<std::size_t>(atlasWidth_ * atlasHeight_), 0);
  glyphs_.clear();

  // Keep glyph coverage separate when the atlas is filtered down to 1/4 size.
  constexpr int gutter = 4;
  int currentX = gutter;
  int currentY = gutter;
  int rowHeight = 0;

  for (uint32_t cp : codepoints) {
    if (FT_Load_Char(face, cp, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT)) {
      continue;
    }

    const auto &glyph = face->glyph;
    const auto &bm = glyph->bitmap;
    const int w = static_cast<int>(bm.width);
    const int h = static_cast<int>(bm.rows);

    GlyphMetrics gm{};
    gm.codepoint = cp;
    gm.advanceX = static_cast<float>(glyph->advance.x) / 64.0F;
    gm.bearingX = static_cast<float>(glyph->bitmap_left);
    gm.bearingY = static_cast<float>(glyph->bitmap_top);
    gm.width = static_cast<float>(w);
    gm.height = static_cast<float>(h);

    if (cp == 'H' || (capHeight_ == 0.0F && (cp == 'M' || cp == 'A' || cp == 'X'))) {
      capHeight_ = gm.height > 0.0F ? gm.height
                                    : (static_cast<float>(glyph->metrics.height) / 64.0F);
    }

    if (w > 0 && h > 0) {
      if (currentX + w + gutter > atlasWidth_) {
        currentX = gutter;
        currentY += rowHeight + gutter;
        rowHeight = 0;
      }
      if (currentY + h + gutter > atlasHeight_) {
        std::fputs("FontAtlas texture atlas full\n", stderr);
        break;
      }

      for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
          const auto srcVal = bm.buffer[row * bm.pitch + col];
          const auto dstIdx = (currentY + row) * atlasWidth_ + (currentX + col);
          bitmap_[static_cast<std::size_t>(dstIdx)] = srcVal;
        }
      }

      gm.u0 = static_cast<float>(currentX) / static_cast<float>(atlasWidth_);
      gm.v0 = static_cast<float>(currentY) / static_cast<float>(atlasHeight_);
      gm.u1 = static_cast<float>(currentX + w) / static_cast<float>(atlasWidth_);
      gm.v1 = static_cast<float>(currentY + h) / static_cast<float>(atlasHeight_);

      currentX += w + gutter;
      rowHeight = std::max(rowHeight, h);
    }

    glyphs_[cp] = gm;
  }

  if (capHeight_ <= 0.0F) {
    capHeight_ = ascent_ * 0.72F;
  }

  FT_Done_Face(face);
  FT_Done_FreeType(ft);
  return true;
}

const GlyphMetrics *FontAtlas::findGlyph(uint32_t codepoint) const {
  const auto it = glyphs_.find(codepoint);
  if (it != glyphs_.end()) {
    return &it->second;
  }
  return nullptr;
}

Vector2 FontAtlas::measure(std::string_view text, float fontSize,
                           float spacing) const {
  if (baseSize_ <= 0 || text.empty()) {
    return {0.0F, 0.0F};
  }

  const float scale = fontSize / static_cast<float>(baseSize_);
  float lineWidth = 0.0F;
  float maxLineWidth = 0.0F;
  int lineCount = 1;
  int charsInLine = 0;

  std::size_t i = 0;
  while (i < text.size()) {
    const uint32_t cp = decodeUtf8(text, i);
    if (cp == '\n') {
      if (charsInLine > 1) {
        lineWidth += static_cast<float>(charsInLine - 1) * spacing;
      }
      maxLineWidth = std::max(maxLineWidth, lineWidth);
      lineWidth = 0.0F;
      charsInLine = 0;
      lineCount++;
    } else {
      const auto *gm = findGlyph(cp);
      if (!gm) {
        gm = findGlyph('?');
      }
      if (gm) {
        lineWidth += gm->advanceX * scale;
      }
      charsInLine++;
    }
  }

  if (charsInLine > 1) {
    lineWidth += static_cast<float>(charsInLine - 1) * spacing;
  }
  maxLineWidth = std::max(maxLineWidth, lineWidth);
  const float textHeight = fontSize + static_cast<float>(lineCount - 1) *
                                          lineAdvance(fontSize);
  return {maxLineWidth, textHeight};
}

float FontAtlas::baselineOffset(float fontSize) const {
  if (baseSize_ <= 0) {
    return fontSize * 0.8F;
  }
  return ascent_ * fontSize / static_cast<float>(baseSize_);
}

float FontAtlas::lineAdvance(float fontSize) const {
  if (baseSize_ <= 0) {
    return fontSize;
  }
  return std::max(fontSize,
                  lineHeight_ * fontSize / static_cast<float>(baseSize_));
}

float FontAtlas::centeredLineTop(float centerY, float fontSize) const {
  if (baseSize_ <= 0) {
    return centerY - fontSize * 0.5F;
  }
  const float capHeight = capHeight_ * fontSize / static_cast<float>(baseSize_);
  return centerY + capHeight * 0.5F - baselineOffset(fontSize);
}

} // namespace workbench
