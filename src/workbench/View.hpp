#pragma once

#include "raylib.h"

#include <string_view>

namespace workbench {

class Session;
struct Layout;

struct Fonts {
  Font ui;
  Font mono;
};

Fonts loadFonts();
void unloadFonts(const Fonts &fonts);
void drawWorkbench(const Session &session, const Layout &layout,
                   const Fonts &fonts, std::string_view status, int width,
                   int height);

} // namespace workbench
