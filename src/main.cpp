#include "bombe/DecipherPanel.hpp"
#include "workbench/AudioEngine.hpp"
#include "workbench/DrawCommand.hpp"
#include "workbench/Input.hpp"
#include "workbench/Layout.hpp"
#include "workbench/Session.hpp"
#include "workbench/View.hpp"
#include "workbench/VulkanRenderer.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>

namespace workbench {

struct EventQueue {
  std::vector<InputEvent> events;
  Vector2 lastCursorPos{};
  bool damaged{true};
  int windowWidth{1320};
  int windowHeight{840};
  int fbWidth{1320};
  int fbHeight{840};
  float dpiScale{1.0F};
  bool dpiChanged{false};
  bool ctrlDown{false};
};

} // namespace workbench

int main(int argc, char **argv) {
  using namespace workbench;

  bool smokeTest = false;
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--smoke-test") {
      smokeTest = true;
    }
  }

  glfwSetErrorCallback([](int error, const char *description) {
    std::fprintf(stderr, "GLFW error [%d]: %s\n", error,
                 description ? description : "unknown");
  });

  if (!glfwInit()) {
    std::fputs("Enigma Workbench could not open a graphics window.\n", stderr);
    return EXIT_FAILURE;
  }

  if (!glfwVulkanSupported()) {
    std::fputs("Enigma Workbench could not open a graphics window.\n", stderr);
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow *window =
      glfwCreateWindow(1320, 840, "Enigma Workbench", nullptr, nullptr);
  if (!window) {
    std::fputs("Enigma Workbench could not open a graphics window.\n", stderr);
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwSetWindowSizeLimits(window, minimumWidth, minimumHeight, GLFW_DONT_CARE,
                          GLFW_DONT_CARE);

  EventQueue queue{};
  glfwGetWindowSize(window, &queue.windowWidth, &queue.windowHeight);
  glfwGetFramebufferSize(window, &queue.fbWidth, &queue.fbHeight);
  {
    float xscale = 1.0F;
    float yscale = 1.0F;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    queue.dpiScale = std::max(xscale, yscale);
  }
  glfwSetWindowUserPointer(window, &queue);

  glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int scancode, int action,
                                int mods) {
    auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
    if (!q) {
      return;
    }
    q->ctrlDown = (static_cast<unsigned>(mods) &
                   static_cast<unsigned>(GLFW_MOD_CONTROL)) != 0;
    q->events.push_back({EventType::Key, key, scancode, action, mods, 0, 0,
                         q->lastCursorPos, 0, 0});
  });

  glfwSetCharCallback(window, [](GLFWwindow *w, unsigned int codepoint) {
    auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
    if (!q) {
      return;
    }
    const int mods = q->ctrlDown ? 2 : 0;
    q->events.push_back({EventType::Char, 0, 0, 0, mods, codepoint, 0,
                         q->lastCursorPos, 0, 0});
  });

  glfwSetMouseButtonCallback(
      window, [](GLFWwindow *w, int button, int action, int mods) {
        auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
        if (!q) {
          return;
        }
        q->events.push_back({EventType::MouseButton, 0, 0, action, mods, 0,
                             button, q->lastCursorPos, 0, 0});
      });

  glfwSetCursorPosCallback(window, [](GLFWwindow *w, double xpos, double ypos) {
    auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
    if (!q) {
      return;
    }
    q->lastCursorPos = {static_cast<float>(xpos), static_cast<float>(ypos)};
    q->events.push_back({EventType::CursorPos, 0, 0, 0, 0, 0, 0,
                         q->lastCursorPos, 0, 0});
  });

  glfwSetWindowSizeCallback(window, [](GLFWwindow *w, int width, int height) {
    auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
    if (!q) {
      return;
    }
    q->windowWidth = width;
    q->windowHeight = height;
    q->events.push_back({EventType::WindowResize, 0, 0, 0, 0, 0, 0, {},
                         width, height});
  });

  glfwSetFramebufferSizeCallback(window, [](GLFWwindow *w, int width,
                                            int height) {
    auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
    if (!q) {
      return;
    }
    q->fbWidth = width;
    q->fbHeight = height;
    q->damaged = true;
  });

  glfwSetWindowContentScaleCallback(
      window, [](GLFWwindow *w, float xscale, float yscale) {
        auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
        if (!q) {
          return;
        }
        const float nextScale = std::max(xscale, yscale);
        if (std::abs(nextScale - q->dpiScale) > 0.01F) {
          q->dpiScale = nextScale;
          q->dpiChanged = true;
          q->damaged = true;
        }
      });

  glfwSetWindowRefreshCallback(window, [](GLFWwindow *w) {
    auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
    if (q) {
      q->damaged = true;
    }
  });

  glfwSetWindowCloseCallback(window, [](GLFWwindow *w) {
    auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
    if (q) {
      q->events.push_back({EventType::WindowClose, 0, 0, 0, 0, 0, 0, {}, 0, 0});
    }
  });

  VulkanRenderer renderer;
  if (!renderer.init(window)) {
    std::fputs("Enigma Workbench could not initialize Vulkan renderer.\n",
               stderr);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }

  Assets assets = loadAssets(renderer, "", queue.dpiScale);
  if (!assets.fontsLoaded) {
    std::fputs("Enigma Workbench could not load any fonts.\n", stderr);
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }
  Session session;
  bombe::DecipherPanel decipher;
  InterfaceState state;
  AudioEngine audio;
  audio.init();
  DrawList drawList;
  bool shouldClose = false;

  Layout layout =
      makeLayout(queue.windowWidth, queue.windowHeight, session.rotorCount());

  PlatformContext platform{
      .setClipboard =
          [window](std::string_view text) {
            std::string nullTerminated(text);
            glfwGetError(nullptr);
            glfwSetClipboardString(window, nullTerminated.c_str());
            return glfwGetError(nullptr) == GLFW_NO_ERROR;
          },
      .getClipboard =
          [window]() -> std::string {
            const char *str = glfwGetClipboardString(window);
            return str ? std::string(str) : "";
          }};

  queue.damaged = true;

  int exitCode = EXIT_SUCCESS;
  int smokeFramesRemaining = smokeTest ? 3 : 0;
  while (!glfwWindowShouldClose(window) && !shouldClose) {
    if (queue.dpiChanged) {
      queue.dpiChanged = false;
      reloadFontAtlases(renderer, assets, queue.dpiScale);
      queue.damaged = true;
    }

    std::vector<InputEvent> currentEvents;
    currentEvents.swap(queue.events);

    bool needRedraw = queue.damaged;
    queue.damaged = false;

    for (const auto &event : currentEvents) {
      if (!state.photoOpen && !state.presetsOpen) {
        const auto rotorCount = session.rotorCount();
        if (const auto handled = decipher.handle(event, platform, session,
                                                  queue.windowWidth, queue.windowHeight)) {
          if (session.rotorCount() != rotorCount) {
            layout = makeLayout(queue.windowWidth, queue.windowHeight, session.rotorCount());
          }
          needRedraw |= *handled;
          continue;
        }
      }
      if (handleEvent(session, layout, state, event, platform, shouldClose, &audio)) {
        needRedraw = true;
      }
    }

    if (shouldClose) {
      break;
    }

    needRedraw |= decipher.update();

    static bool wasStatusActive = false;
    const bool isStatusActive = !state.status.isExpired();
    if (wasStatusActive && !isStatusActive) {
      needRedraw = true;
    }
    wasStatusActive = isStatusActive;

    if (needRedraw && queue.fbWidth > 0 && queue.fbHeight > 0) {
      if (renderer.beginFrame(queue.fbWidth, queue.fbHeight)) {
        drawList.clear();
        if (!decipher.isOpen()) {
          drawWorkbench(session, layout, assets, state, queue.lastCursorPos,
                        drawList, queue.windowWidth, queue.windowHeight, &audio);
        }
        if (!state.photoOpen) {
          const auto cursor = state.presetsOpen ? Vector2{-1, -1} : queue.lastCursorPos;
          decipher.draw(drawList, assets, cursor, queue.windowWidth,
                        queue.windowHeight);
        }
        renderer.renderDrawList(drawList, queue.windowWidth, queue.windowHeight);
        if (!renderer.endFrame()) {
          exitCode = EXIT_FAILURE;
          break;
        }
        queue.damaged = renderer.needsRedraw();
        if (smokeTest) {
          if (--smokeFramesRemaining <= 0) {
            shouldClose = true;
          } else {
            queue.damaged = true;
          }
        }
      } else if (renderer.hasFailed()) {
        exitCode = EXIT_FAILURE;
        break;
      } else {
        queue.damaged = true;
      }
    }

    if (queue.events.empty()) {
      if (queue.damaged && queue.fbWidth > 0 && queue.fbHeight > 0) {
        glfwWaitEventsTimeout(0.016);
      } else if (decipher.isRunning() && !decipher.isPaused()) {
        glfwWaitEventsTimeout(0.04);
      } else if (!state.status.isExpired()) {
        const double timeout = state.status.secondsRemaining();
        if (timeout > 0.0) {
          glfwWaitEventsTimeout(std::min(timeout, 0.05));
        } else {
          glfwPollEvents();
        }
      } else {
        glfwWaitEvents();
      }
    } else {
      glfwPollEvents();
    }
  }

  audio.shutdown();
  unloadAssets(renderer, assets);
  renderer.shutdown();
  glfwDestroyWindow(window);
  glfwTerminate();
  return exitCode;
}
