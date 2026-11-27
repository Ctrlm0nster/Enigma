#include "bombe/DecipherPanel.hpp"
#include "workbench/DrawCommand.hpp"
#include "workbench/Input.hpp"
#include "workbench/Layout.hpp"
#include "workbench/Session.hpp"
#include "workbench/View.hpp"
#include "workbench/VulkanRenderer.hpp"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
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
  bool ctrlDown{false};
};

} // namespace workbench

int main() {
  using namespace workbench;

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
  glfwSetWindowUserPointer(window, &queue);

  glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int scancode, int action,
                                int mods) {
    auto *q = static_cast<EventQueue *>(glfwGetWindowUserPointer(w));
    if (!q) {
      return;
    }
    q->ctrlDown = (mods & GLFW_MOD_CONTROL) != 0;
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

  const Assets assets = loadAssets(renderer);
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
  DrawList drawList;
  bool shouldClose = false;

  Layout layout =
      makeLayout(queue.windowWidth, queue.windowHeight, session.rotorCount());

  PlatformContext platform{
      .setClipboard =
          [window](std::string_view text) {
            std::string nullTerminated(text);
            glfwSetClipboardString(window, nullTerminated.c_str());
          },
      .getClipboard =
          [window]() -> std::string {
            const char *str = glfwGetClipboardString(window);
            return str ? std::string(str) : "";
          }};

  queue.damaged = true;

  while (!glfwWindowShouldClose(window) && !shouldClose) {
    std::vector<InputEvent> currentEvents;
    currentEvents.swap(queue.events);

    bool needRedraw = queue.damaged;
    queue.damaged = false;

    for (const auto &event : currentEvents) {
      if (!state.photoOpen) {
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
      if (handleEvent(session, layout, state, event, platform, shouldClose)) {
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
                        drawList, queue.windowWidth, queue.windowHeight);
        }
        if (!state.photoOpen) {
          decipher.draw(drawList, assets, queue.lastCursorPos, queue.windowWidth,
                        queue.windowHeight);
        }
        renderer.renderDrawList(drawList, queue.windowWidth, queue.windowHeight);
        if (!renderer.endFrame()) {
          shouldClose = true;
          break;
        }
      }
    }

    if (queue.events.empty()) {
      if (decipher.isRunning()) {
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

  unloadAssets(renderer, assets);
  renderer.shutdown();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
