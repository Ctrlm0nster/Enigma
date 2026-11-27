#include "bombe/DecipherPanel.hpp"
#include "workbench/Input.hpp"
#include "workbench/Layout.hpp"
#include "workbench/Session.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
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
  key(GLFW_KEY_F7);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (panel.isRunning() && !panel.isPaused() &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    panel.update();
  }
  REQUIRE(panel.isPaused());
  REQUIRE(panel.isRunning());
  clipboard.clear();
  key(GLFW_KEY_C, GLFW_MOD_CONTROL);
  REQUIRE(clipboard == "AAAAA");

  key(GLFW_KEY_F6);
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
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

  session.loadPreset(1);
  REQUIRE(session.rotorCount() == 4);
  session.clearMessage();
  REQUIRE(session.paste("NORTHATLANTIC") == 13);
  const auto m4Ciphertext = session.output();
  clipboard = m4Ciphertext;
  click(807, 45);
  click(90, 105);
  key(GLFW_KEY_A, GLFW_MOD_CONTROL);
  key(GLFW_KEY_V, GLFW_MOD_CONTROL);
  click(400, 161);
  key(GLFW_KEY_ENTER);
  clipboard.clear();
  key(GLFW_KEY_C, GLFW_MOD_CONTROL);
  CHECK(clipboard == "NORTHATLANTIC");
  CHECK(session.output() == m4Ciphertext);
}

TEST_CASE("Bombe challenges can be stepped, cancelled, and solved",
          "[bombe][workbench]") {
  using namespace workbench;
  bombe::DecipherPanel panel;
  Session session;
  std::string clipboard;
  const PlatformContext platform{
      .setClipboard = [&](std::string_view text) { clipboard = text; },
      .getClipboard = [&] { return clipboard; }};
  const auto send = [&](const InputEvent &event) {
    REQUIRE(panel.handle(event, platform, session, minimumWidth, minimumHeight));
  };
  const auto click = [&](float x, float y) {
    send({.type = EventType::MouseButton, .action = GLFW_PRESS,
          .button = GLFW_MOUSE_BUTTON_LEFT, .cursor = {x, y}});
  };
  const auto press = [&](int key, int mods = 0) {
    send({.type = EventType::Key, .key = key, .action = GLFW_PRESS, .mods = mods});
  };
  const auto waitFor = [&](const auto &condition) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!condition() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      panel.update();
    }
    REQUIRE(condition());
  };

  click(807, 43);
  click(220, 104);
  SECTION("Clicking the pairs field in Unknown mode fixes the entered pair") {
    session.clearPlugboard();
    const std::string message = "MEETATTHEWEATHERSTATIONATDAWNBRINGEXTRASUPPLIES";
    enigma::MachineSettings settings;
    settings.plugPairs = {{0, 25}};
    clipboard = enigma::makeMachine(settings).processString(message);
    press(GLFW_KEY_V, GLFW_MOD_CONTROL);
    click(110, 350);
    clipboard = message;
    press(GLFW_KEY_V, GLFW_MOD_CONTROL);
    click(110, 565);
    clipboard = "AZ";
    press(GLFW_KEY_V, GLFW_MOD_CONTROL);
    press(GLFW_KEY_F7);
    waitFor([&] { return panel.isPaused(); });
    click(735, 635);
    const auto key = session.settings();
    CHECK(std::ranges::find(key.plugPairs, enigma::PlugPair{0, 25}) !=
          key.plugPairs.end());
  }
  SECTION("Generated challenge") {
    click(400, 104);
    press(GLFW_KEY_F7);
    waitFor([&] { return panel.isPaused(); });
    REQUIRE(panel.isRunning());
    press(GLFW_KEY_F7);
    CHECK_FALSE(panel.isPaused());
    waitFor([&] { return panel.isPaused(); });
    click(420, 635);
    waitFor([&] { return !panel.isRunning(); });

    press(GLFW_KEY_F7);
    waitFor([&] { return panel.isPaused(); });
    press(GLFW_KEY_F6);
    waitFor([&] { return !panel.isRunning(); });
    press(GLFW_KEY_C, GLFW_MOD_CONTROL);
    CHECK(clipboard == "MEETATTHEWEATHERSTATIONATDAWNBRINGEXTRASUPPLIESFORTHETEAM"
                       "THENWAITFORTHERADIOOPERATORINTHELOOKOUTTOWER");
    click(735, 635);
    CHECK(session.plugPairs().size() == 3);
  }
}
