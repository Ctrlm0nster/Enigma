#include "bombe/DecipherPanel.hpp"
#include "workbench/Input.hpp"
#include "workbench/Layout.hpp"
#include "workbench/Session.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

TEST_CASE(
    "Decipher panel copies plaintext and applies a recovered key explicitly",
    "[bombe][workbench]") {
  using namespace workbench;
  bombe::DecipherPanel panel;
  Session session;
  REQUIRE(session.setRotorCount(5));
  session.adjustRotor(4, 1, 2);
  REQUIRE(session.paste("ENIGMA") == 6);
  const auto originalOutput = session.output();

  std::string clipboard = "BDZ GO";
  PlatformContext platform{
      .setClipboard = [&](std::string_view text) { clipboard = text; },
      .getClipboard = [&] { return clipboard; }};
  const auto send = [&](const InputEvent &event) {
    const auto consumed =
        panel.handle(event, platform, session, minimumWidth, minimumHeight);
    REQUIRE(consumed.has_value());
  };
  const auto click = [&](float x, float y) {
    send({.type = EventType::MouseButton,
          .action = GLFW_PRESS,
          .button = GLFW_MOUSE_BUTTON_LEFT,
          .cursor = {x, y}});
  };
  const auto key = [&](int pressed, int mods = 0) {
    send({.type = EventType::Key,
          .key = pressed,
          .action = GLFW_PRESS,
          .mods = mods});
  };

  click(807, 45);
  REQUIRE(panel.isOpen());
  key(GLFW_KEY_V, GLFW_MOD_CONTROL);
  key(GLFW_KEY_ENTER);
  key(GLFW_KEY_C, GLFW_MOD_CONTROL);
  CHECK(clipboard == "AAAAA");
  CHECK(session.output() == originalOutput);

  click(220, 105);
  click(110, 350);
  key(GLFW_KEY_V, GLFW_MOD_CONTROL);
  click(410, 505);
  key(GLFW_KEY_ENTER);
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (panel.isRunning() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    panel.update();
  }
  REQUIRE_FALSE(panel.isRunning());
  clipboard.clear();
  key(GLFW_KEY_C, GLFW_MOD_CONTROL);
  REQUIRE(clipboard == "AAAAA");
  CHECK(session.rotorCount() == 5);
  CHECK(session.output() == originalOutput);

  click(735, 635);
  CHECK(session.rotorCount() == 3);
  CHECK(session.input() == "ENIGMA");
  CHECK(session.output() == enigma::makeMachine({}).processString("ENIGMA"));
  key(GLFW_KEY_ESCAPE);
  CHECK_FALSE(panel.isOpen());
  REQUIRE(session.setRotorCount(5));
  CHECK(session.startAt(4) == 1);
  CHECK(session.ringAt(4) == 2);
}
