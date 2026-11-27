/**
 * @file View.hpp
 * @brief Workbench view rendering routines and asset management.
 */

#pragma once

#include "DrawCommand.hpp"
#include "FontAtlas.hpp"
#include "Geometry.hpp"

#include <string>

namespace workbench {

class Session;
struct Layout;
struct InterfaceState;
class VulkanRenderer;
class AudioEngine;

/**
 * @struct Assets
 * @brief Bundled fonts and graphic textures used by the workbench interface.
 */
struct Assets {
  FontAtlas title;
  FontAtlas ui;
  FontAtlas mono;
  uint32_t photoTextureId{0};
  int photoWidth{0};
  int photoHeight{0};
  bool photoAvailable{false};
  bool fontsLoaded{false};
};

/**
 * @brief Resolves the directory where assets reside.
 */
std::string resolveAssetDirectory();

/**
 * @brief Loads a font with automatic fallback to other bundled fonts if primary is missing.
 */
bool loadFontWithFallback(FontAtlas &atlas, const std::string &root,
                          const std::string &primaryFace, int size,
                          std::span<const uint32_t> extraCodepoints = {});

/**
 * @brief Loads application fonts and texture assets and uploads them to the GPU.
 */
Assets loadAssets(VulkanRenderer &renderer, const std::string &assetDir = "");

/**
 * @brief Unloads graphics and fonts from GPU memory upon exit.
 */
void unloadAssets(VulkanRenderer &renderer, const Assets &assets);

/**
 * @brief Draws the entire workbench screen by emitting 2D commands into DrawList.
 */
void drawWorkbench(const Session &session, const Layout &layout,
                   const Assets &assets, const InterfaceState &state,
                   Vector2 cursor, DrawList &drawList, int screenWidth,
                   int screenHeight, const AudioEngine *audio = nullptr);

} // namespace workbench
