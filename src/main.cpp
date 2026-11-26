#include "workbench/Input.hpp"
#include "workbench/Layout.hpp"
#include "workbench/Session.hpp"
#include "workbench/View.hpp"

int main() {
  using namespace workbench;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
  InitWindow(1280, 820, "Enigma Workbench");
  SetWindowMinSize(minimumWidth, minimumHeight);
  SetTargetFPS(60);

  const Fonts fonts = loadFonts();
  Session session;
  Status status;
  while (!WindowShouldClose()) {
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    const Layout layout = makeLayout(width, height);
    handleInput(session, layout, status);

    BeginDrawing();
    drawWorkbench(session, layout, fonts, status.current(), width, height);
    EndDrawing();
  }

  unloadFonts(fonts);
  CloseWindow();
  return 0;
}
