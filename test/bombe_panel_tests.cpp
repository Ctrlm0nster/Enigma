#include "bombe/DecipherPanel.hpp"
#include "bombe/DecipherInput.hpp"
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
      .setClipboard = [&](std::string_view text) { clipboard = text; return true; },
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
      .setClipboard = [&](std::string_view text) { clipboard = text; return true; },
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
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
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

TEST_CASE("Workbench challenges preview, copy and import without changing either session",
          "[bombe][workbench]") {
  using namespace workbench;
  const std::string message = "CHECKTHEGAUGESBEFORETHEMORNINGSHIFTANDRECORDTHEREADINGS";
  auto key = bombe::parseSettings("MCK", "03 02 05", "AZ BY CR");
  Session sender;
  sender.applySettings(key);
  bombe::DecipherPanel author;
  Session receiver;
  REQUIRE(receiver.setRotorCount(4));
  REQUIRE(receiver.paste("OTHERMESSAGE") == 12);
  const auto receiverOutput = receiver.output();
  bombe::DecipherPanel reader;

  std::string clipboard;
  bool copyAvailable = true;
  const PlatformContext platform{
      .setClipboard = [&](std::string_view text) {
        if (copyAvailable) {
          clipboard = text;
        }
        return copyAvailable;
      },
      .getClipboard = [&] { return clipboard; }};
  const auto send = [&](auto &panel, auto &session, const InputEvent &event) {
    return panel.handle(event, platform, session, minimumWidth, minimumHeight);
  };
  const auto clickEvent = [](Vector2 cursor) {
    return InputEvent{.type = EventType::MouseButton, .action = GLFW_PRESS,
                      .button = GLFW_MOUSE_BUTTON_LEFT, .cursor = cursor};
  };
  const auto click = [&](auto &panel, auto &session, Vector2 cursor) {
    REQUIRE(send(panel, session, clickEvent(cursor)).value_or(false));
  };
  const auto press = [&](auto &panel, auto &session, int code, int mods = 0) {
    REQUIRE(send(panel, session, {.type = EventType::Key, .key = code,
                                  .action = GLFW_PRESS, .mods = mods}).value_or(false));
  };

  const auto layout = makeLayout(minimumWidth, minimumHeight, sender.rotorCount());
  const auto create = center(layout.createChallenge);
  CHECK_FALSE(send(author, sender, clickEvent(create)).has_value());
  REQUIRE(sender.paste(message) == message.size());
  const auto ciphertext = sender.output();
  click(author, sender, create);
  REQUIRE(author.isOpen());
  click(author, sender, {555, 105});
  clipboard = "empty crib";
  press(author, sender, GLFW_KEY_C, GLFW_MOD_CONTROL);
  CHECK(clipboard == "empty crib");

  const auto crib = message.substr(0, 41);
  clipboard = crib;
  press(author, sender, GLFW_KEY_V, GLFW_MOD_CONTROL);
  click(author, sender, {110, 565});
  clipboard = "ZA";
  press(author, sender, GLFW_KEY_V, GLFW_MOD_CONTROL);
  click(author, sender, {555, 105});
  copyAvailable = false;
  press(author, sender, GLFW_KEY_C, GLFW_MOD_CONTROL);
  CHECK(clipboard == "ZA");
  copyAvailable = true;
  press(author, sender, GLFW_KEY_C, GLFW_MOD_CONTROL);
  const auto partial = bombe::decodeChallenge(clipboard);
  CHECK(partial.ciphertext == ciphertext);
  CHECK(partial.crib == crib);
  CHECK_FALSE(partial.cribOffset.has_value());
  CHECK(partial.rotors == bombe::RotorScope::FirstFive);
  CHECK(partial.reflectors == bombe::ReflectorScope::Both);
  CHECK(partial.plugboard == bombe::PlugboardMode::Partial);
  CHECK(bombe::formatLeads(partial.knownLeads) == "ZA");
  CHECK(clipboard.ends_with("\nPairs=ZA"));

  press(author, sender, GLFW_KEY_ESCAPE);
  REQUIRE(author.isOpen());
  click(author, sender, {400, 161});
  click(author, sender, {85, 468});
  click(author, sender, {425, 468});
  click(author, sender, {385, 430});
  clipboard = "1";
  press(author, sender, GLFW_KEY_V, GLFW_MOD_CONTROL);
  click(author, sender, {555, 105});
  press(author, sender, GLFW_KEY_C, GLFW_MOD_CONTROL);
  const auto payload = clipboard;
  const auto shared = bombe::decodeChallenge(payload);
  CHECK(shared.plugboard == bombe::PlugboardMode::Known);
  CHECK(shared.rotors == bombe::RotorScope::Fixed);
  CHECK(shared.reflectors == bombe::ReflectorScope::B);
  CHECK(shared.cribOffset == 0);
  CHECK(payload.find(message) == std::string::npos);
  CHECK(payload.find("Start=") == std::string::npos);
  CHECK(sender.output() == ciphertext);
  CHECK(bombe::formatStarts(sender.settings()) == "MCK");

  click(reader, receiver, {807, 43});
  click(reader, receiver, {220, 104});
  click(reader, receiver, {707, 104});
  clipboard = payload + "\nCrib=WRONG";
  click(reader, receiver, {707, 104});
  press(reader, receiver, GLFW_KEY_ENTER);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (reader.isRunning() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    reader.update();
  }
  REQUIRE_FALSE(reader.isRunning());
  press(reader, receiver, GLFW_KEY_C, GLFW_MOD_CONTROL);
  CHECK(clipboard == message);
  CHECK(receiver.rotorCount() == 4);
  CHECK(receiver.output() == receiverOutput);
}
