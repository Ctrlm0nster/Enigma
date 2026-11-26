#include "workbench/Input.hpp"
#include "workbench/Layout.hpp"
#include "workbench/Session.hpp"
#include "workbench/View.hpp"

#include <cstdio>
#include <cstdlib>

int main() {
  using namespace workbench;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
  InitWindow(1320, 840, "Enigma Workbench");
  if (!IsWindowReady()) {
    std::fputs("Enigma Workbench could not open a graphics window.\n", stderr);
    return EXIT_FAILURE;
  }
  SetWindowMinSize(minimumWidth, minimumHeight);
  SetExitKey(KEY_NULL);
  SetTargetFPS(60);

  const Assets assets = loadAssets();
  Session session;
  InterfaceState state;
  while (!WindowShouldClose() &&
         (state.photoOpen || !IsKeyPressed(KEY_ESCAPE))) {
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    const auto rotorCount = session.rotorCount();
    Layout layout = makeLayout(width, height, rotorCount);
    handleInput(session, layout, state);
    if (session.rotorCount() != rotorCount) {
      layout = makeLayout(width, height, session.rotorCount());
    }

    BeginDrawing();
    drawWorkbench(session, layout, assets, state);
    EndDrawing();
  }

  unloadAssets(assets);
  CloseWindow();
  return 0;
}
