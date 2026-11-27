/**
 * @file DrawCommand.hpp
 * @brief 2D batching and tessellation for Enigma Workbench.
 */

#pragma once

#include "FontAtlas.hpp"
#include "Geometry.hpp"

#include <numbers>
#include <vector>

namespace workbench {

struct Vertex {
  float pos[2]{0.0F, 0.0F};
  float uv[2]{0.0F, 0.0F};
  float color[4]{1.0F, 1.0F, 1.0F, 1.0F};
};

enum class PipelineType { Solid, Sampled };

struct DrawBatch {
  PipelineType pipeline{PipelineType::Solid};
  uint32_t textureId{0};
  Rectangle scissor{}; // Empty width/height means full viewport
  uint32_t firstIndex{0};
  uint32_t indexCount{0};
};

class DrawList {
public:
  DrawList();
  ~DrawList();

  void clear();

  void setPipeline(PipelineType pipeline, uint32_t textureId = 0);
  void setScissor(Rectangle scissor);
  void resetScissor();

  void addRectangle(Rectangle bounds, Color color);
  void addRectangleRounded(Rectangle bounds, float roundness, int segments,
                           Color color);
  void addBox(Rectangle bounds, Color fill, Color edge = border,
              float roundness = 0.07F);
  void addLine(Vector2 start, Vector2 end, float thickness, Color color);
  void addCircle(Vector2 center, float radius, Color color, int segments = 32);
  void addBloomDisc(Vector2 center, float innerRadius, float outerRadius,
                    Color innerColor, Color outerColor, int segments = 24);
  void addCubicBezier(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3,
                      float thickness, Color color, int segments = 24);
  void addTexturedQuad(uint32_t textureId, Rectangle src, Rectangle dst,
                       Color tint, Vector2 texSize);
  void addText(const FontAtlas &font, std::string_view text, Vector2 origin,
               float size, Color color = foreground, float spacing = 0.0F);
  void addCenteredText(const FontAtlas &font, std::string_view text,
                       Vector2 point, float size, Color color = foreground);

  const std::vector<Vertex> &vertices() const { return vertices_; }
  const std::vector<uint32_t> &indices() const { return indices_; }
  const std::vector<DrawBatch> &batches() const { return batches_; }

private:
  void ensureBatch();

  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;
  std::vector<DrawBatch> batches_;

  PipelineType currentPipeline_{PipelineType::Solid};
  uint32_t currentTextureId_{0};
  Rectangle currentScissor_{};
};

} // namespace workbench
