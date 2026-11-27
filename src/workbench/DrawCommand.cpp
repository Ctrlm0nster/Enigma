#include "DrawCommand.hpp"

#include <cmath>

namespace workbench {

DrawList::DrawList() = default;
DrawList::~DrawList() = default;

void DrawList::clear() {
  vertices_.clear();
  indices_.clear();
  batches_.clear();
  currentPipeline_ = PipelineType::Solid;
  currentTextureId_ = 0;
  currentScissor_ = {};
}

void DrawList::setPipeline(PipelineType pipeline, uint32_t textureId) {
  if (currentPipeline_ != pipeline || currentTextureId_ != textureId) {
    currentPipeline_ = pipeline;
    currentTextureId_ = textureId;
    if (!batches_.empty() && batches_.back().indexCount == 0) {
      batches_.back().pipeline = currentPipeline_;
      batches_.back().textureId = currentTextureId_;
    } else {
      ensureBatch();
    }
  }
}

void DrawList::setScissor(Rectangle scissor) {
  if (currentScissor_ != scissor) {
    currentScissor_ = scissor;
    ensureBatch();
  }
}

void DrawList::resetScissor() { setScissor(Rectangle{}); }

void DrawList::ensureBatch() {
  if (!batches_.empty()) {
    auto &last = batches_.back();
    if (last.indexCount == 0) {
      last.pipeline = currentPipeline_;
      last.textureId = currentTextureId_;
      last.scissor = currentScissor_;
      return;
    }
    if (last.pipeline == currentPipeline_ &&
        last.textureId == currentTextureId_ &&
        last.scissor == currentScissor_) {
      return;
    }
  }

  DrawBatch batch{};
  batch.pipeline = currentPipeline_;
  batch.textureId = currentTextureId_;
  batch.scissor = currentScissor_;
  batch.firstIndex = static_cast<uint32_t>(indices_.size());
  batch.indexCount = 0;
  batches_.push_back(batch);
}

void DrawList::addRectangle(Rectangle bounds, Color color) {
  setPipeline(PipelineType::Solid, 0);
  ensureBatch();

  const auto c = colorToLinear(color);
  const auto base = static_cast<uint32_t>(vertices_.size());

  vertices_.push_back({{bounds.x, bounds.y}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{bounds.x + bounds.width, bounds.y}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{bounds.x + bounds.width, bounds.y + bounds.height}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{bounds.x, bounds.y + bounds.height}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});

  indices_.push_back(base + 0);
  indices_.push_back(base + 1);
  indices_.push_back(base + 2);
  indices_.push_back(base + 0);
  indices_.push_back(base + 2);
  indices_.push_back(base + 3);

  batches_.back().indexCount += 6;
}

void DrawList::addRectangleRounded(Rectangle bounds, float roundness,
                                   int segments, Color color) {
  if (roundness <= 0.0F || bounds.width <= 0.0F || bounds.height <= 0.0F) {
    addRectangle(bounds, color);
    return;
  }

  const float radius = std::min(bounds.width, bounds.height) * 0.5F *
                       std::clamp(roundness, 0.0F, 1.0F);
  if (radius <= 0.5F) {
    addRectangle(bounds, color);
    return;
  }

  if (segments < 4) {
    segments = 8;
  }

  setPipeline(PipelineType::Solid, 0);
  ensureBatch();

  // Five central & edge rectangles
  const Rectangle centerRect{bounds.x + radius, bounds.y + radius,
                             bounds.width - 2.0F * radius,
                             bounds.height - 2.0F * radius};
  const Rectangle topRect{bounds.x + radius, bounds.y,
                          bounds.width - 2.0F * radius, radius};
  const Rectangle bottomRect{bounds.x + radius,
                             bounds.y + bounds.height - radius,
                             bounds.width - 2.0F * radius, radius};
  const Rectangle leftRect{bounds.x, bounds.y + radius, radius,
                           bounds.height - 2.0F * radius};
  const Rectangle rightRect{bounds.x + bounds.width - radius,
                            bounds.y + radius, radius,
                            bounds.height - 2.0F * radius};

  if (centerRect.width > 0.0F && centerRect.height > 0.0F) {
    addRectangle(centerRect, color);
  }
  if (topRect.width > 0.0F && topRect.height > 0.0F) {
    addRectangle(topRect, color);
  }
  if (bottomRect.width > 0.0F && bottomRect.height > 0.0F) {
    addRectangle(bottomRect, color);
  }
  if (leftRect.width > 0.0F && leftRect.height > 0.0F) {
    addRectangle(leftRect, color);
  }
  if (rightRect.width > 0.0F && rightRect.height > 0.0F) {
    addRectangle(rightRect, color);
  }

  // Four rounded corners
  constexpr float pi = std::numbers::pi_v<float>;
  const struct Corner {
    Vector2 center;
    float startAngle;
    float endAngle;
  } corners[4] = {
      {{bounds.x + radius, bounds.y + radius}, pi, 1.5F * pi},
      {{bounds.x + bounds.width - radius, bounds.y + radius}, 1.5F * pi, 2.0F * pi},
      {{bounds.x + bounds.width - radius, bounds.y + bounds.height - radius}, 0.0F, 0.5F * pi},
      {{bounds.x + radius, bounds.y + bounds.height - radius}, 0.5F * pi, pi}};

  const auto c = colorToLinear(color);
  for (const auto &corner : corners) {
    const auto centerIdx = static_cast<uint32_t>(vertices_.size());
    vertices_.push_back({{corner.center.x, corner.center.y},
                         {0.0F, 0.0F},
                         {c[0], c[1], c[2], c[3]}});

    for (int i = 0; i <= segments; ++i) {
      const float t = static_cast<float>(i) / static_cast<float>(segments);
      const float angle = corner.startAngle + t * (corner.endAngle - corner.startAngle);
      vertices_.push_back(
          {{corner.center.x + std::cos(angle) * radius,
            corner.center.y + std::sin(angle) * radius},
           {0.0F, 0.0F},
           {c[0], c[1], c[2], c[3]}});
    }

    for (int i = 0; i < segments; ++i) {
      indices_.push_back(centerIdx);
      indices_.push_back(centerIdx + 1 + static_cast<uint32_t>(i));
      indices_.push_back(centerIdx + 2 + static_cast<uint32_t>(i));
      batches_.back().indexCount += 3;
    }
  }
}

void DrawList::addBox(Rectangle bounds, Color fill, Color edge,
                      float roundness) {
  addRectangleRounded(bounds, roundness, 8, edge);
  addRectangleRounded(insetBy(bounds, 1), roundness, 8, fill);
}

void DrawList::addLine(Vector2 start, Vector2 end, float thickness,
                       Color color) {
  const float dx = end.x - start.x;
  const float dy = end.y - start.y;
  const float len = std::hypot(dx, dy);
  if (len <= 1e-6F) {
    return;
  }

  setPipeline(PipelineType::Solid, 0);
  ensureBatch();

  const float halfThick = thickness * 0.5F;
  const float nx = -dy / len * halfThick;
  const float ny = dx / len * halfThick;

  const auto c = colorToLinear(color);
  const auto base = static_cast<uint32_t>(vertices_.size());

  vertices_.push_back({{start.x + nx, start.y + ny}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{end.x + nx, end.y + ny}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{end.x - nx, end.y - ny}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{start.x - nx, start.y - ny}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});

  indices_.push_back(base + 0);
  indices_.push_back(base + 1);
  indices_.push_back(base + 2);
  indices_.push_back(base + 0);
  indices_.push_back(base + 2);
  indices_.push_back(base + 3);

  batches_.back().indexCount += 6;
}

void DrawList::addCircle(Vector2 center, float radius, Color color,
                         int segments) {
  if (radius <= 0.0F) {
    return;
  }
  if (segments < 8) {
    segments = 8;
  }

  setPipeline(PipelineType::Solid, 0);
  ensureBatch();

  constexpr float twoPi = 2.0F * std::numbers::pi_v<float>;
  const auto c = colorToLinear(color);
  const auto centerIdx = static_cast<uint32_t>(vertices_.size());

  vertices_.push_back({{center.x, center.y}, {0.0F, 0.0F}, {c[0], c[1], c[2], c[3]}});

  for (int i = 0; i < segments; ++i) {
    const float angle = (twoPi * static_cast<float>(i)) / static_cast<float>(segments);
    vertices_.push_back({{center.x + std::cos(angle) * radius,
                          center.y + std::sin(angle) * radius},
                         {0.0F, 0.0F},
                         {c[0], c[1], c[2], c[3]}});
  }

  for (int i = 0; i < segments; ++i) {
    indices_.push_back(centerIdx);
    indices_.push_back(centerIdx + 1 + static_cast<uint32_t>(i));
    indices_.push_back(centerIdx + 1 + static_cast<uint32_t>((i + 1) % segments));
    batches_.back().indexCount += 3;
  }
}

void DrawList::addTexturedQuad(uint32_t textureId, Rectangle src, Rectangle dst,
                               Color tint, Vector2 texSize) {
  if (texSize.x <= 0.0F || texSize.y <= 0.0F) {
    return;
  }

  setPipeline(PipelineType::Sampled, textureId);
  ensureBatch();

  const float u0 = src.x / texSize.x;
  const float v0 = src.y / texSize.y;
  const float u1 = (src.x + src.width) / texSize.x;
  const float v1 = (src.y + src.height) / texSize.y;

  const auto c = colorToLinear(tint);
  const auto base = static_cast<uint32_t>(vertices_.size());

  vertices_.push_back({{dst.x, dst.y}, {u0, v0}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{dst.x + dst.width, dst.y}, {u1, v0}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{dst.x + dst.width, dst.y + dst.height}, {u1, v1}, {c[0], c[1], c[2], c[3]}});
  vertices_.push_back({{dst.x, dst.y + dst.height}, {u0, v1}, {c[0], c[1], c[2], c[3]}});

  indices_.push_back(base + 0);
  indices_.push_back(base + 1);
  indices_.push_back(base + 2);
  indices_.push_back(base + 0);
  indices_.push_back(base + 2);
  indices_.push_back(base + 3);

  batches_.back().indexCount += 6;
}

void DrawList::addText(const FontAtlas &font, std::string_view text,
                       Vector2 origin, float size, Color color,
                       float spacing) {
  if (font.baseSize() <= 0 || text.empty()) {
    return;
  }

  setPipeline(PipelineType::Sampled, font.textureId());
  ensureBatch();

  const float scale = size / static_cast<float>(font.baseSize());
  const auto c = colorToLinear(color);
  float curX = origin.x;
  float baseline = origin.y + font.baselineOffset(size);
  const float lineAdvance = font.lineAdvance(size);

  std::size_t i = 0;
  while (i < text.size()) {
    const uint32_t cp = decodeUtf8(text, i);
    if (cp == '\n') {
      baseline += lineAdvance;
      curX = origin.x;
      continue;
    }

    const auto *gm = font.findGlyph(cp);
    if (!gm) {
      gm = font.findGlyph('?');
    }

    if (gm) {
      if (gm->width > 0.0F && gm->height > 0.0F) {
        const float left = curX + gm->bearingX * scale;
        const float top = baseline - gm->bearingY * scale;
        const float right = left + gm->width * scale;
        const float bottom = top + gm->height * scale;

        const auto base = static_cast<uint32_t>(vertices_.size());
        vertices_.push_back({{left, top}, {gm->u0, gm->v0}, {c[0], c[1], c[2], c[3]}});
        vertices_.push_back({{right, top}, {gm->u1, gm->v0}, {c[0], c[1], c[2], c[3]}});
        vertices_.push_back({{right, bottom}, {gm->u1, gm->v1}, {c[0], c[1], c[2], c[3]}});
        vertices_.push_back({{left, bottom}, {gm->u0, gm->v1}, {c[0], c[1], c[2], c[3]}});

        indices_.push_back(base + 0);
        indices_.push_back(base + 1);
        indices_.push_back(base + 2);
        indices_.push_back(base + 0);
        indices_.push_back(base + 2);
        indices_.push_back(base + 3);

        batches_.back().indexCount += 6;
      }
      curX += (gm->advanceX * scale + spacing);
    }
  }
}

void DrawList::addCenteredText(const FontAtlas &font, std::string_view text,
                               Vector2 point, float size, Color color) {
  addText(font, text,
          {point.x - font.measure(text, size).x * 0.5F,
           font.centeredLineTop(point.y, size)},
          size, color);
}

} // namespace workbench
