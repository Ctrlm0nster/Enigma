/**
 * @file View.hpp
 * @brief Raylib rendering routines and asset management for Enigma Workbench.
 */

#pragma once

#include "raylib.h"

namespace workbench {

class Session;
struct Layout;
struct InterfaceState;

/**
 * @struct Assets
 * @brief Bundled fonts and graphic textures used by the workbench interface.
 */
struct Assets {
  Font title;       ///< Serif headline font (Instrument Serif).
  Font ui;          ///< Modern sans font (Montserrat).
  Font mono;        ///< Monospace telemetry and rotor font (JetBrains Mono).
  Texture2D photo;  ///< Historical Enigma photograph from Imperial War Museum.
};

/**
 * @brief Loads application fonts and texture assets from the assets directory.
 *
 * @return Populated Assets structure.
 */
Assets loadAssets();

/**
 * @brief Unloads graphics and fonts from GPU/CPU memory upon exit.
 *
 * @param assets Reference to assets structure.
 */
void unloadAssets(const Assets &assets);

/**
 * @brief Draws the entire workbench screen in a single frame pass.
 *
 * Renders message readouts, electrical signal trace, lampboard, keyboard,
 * rotor adjustment controls, plugboard sockets and wires, and modal overlays.
 *
 * @param session Current simulation session.
 * @param layout Geometry bounding boxes.
 * @param assets Loaded fonts and textures.
 * @param state Active status and modal visibility.
 */
void drawWorkbench(const Session &session, const Layout &layout,
                   const Assets &assets, const InterfaceState &state);

} // namespace workbench
