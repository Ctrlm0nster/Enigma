#include "Charactermap.hpp"
#include "EnigmaMachine.hpp"
#include "workbench/Session.hpp"
#ifndef ENIGMA_CORE_ONLY
#include "workbench/DrawCommand.hpp"
#include "workbench/FontAtlas.hpp"
#include "workbench/Geometry.hpp"
#include "workbench/ImageDecoder.hpp"
#include "workbench/Input.hpp"
#include "workbench/Layout.hpp"
#include "workbench/View.hpp"
#include "workbench/VulkanRenderer.hpp"
#endif
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <stdexcept>

TEST_CASE("Enigma Reversibility", "[enigma]") {
  Rotor rotor1("EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q');
  Rotor rotor2("AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E');
  Rotor rotor3("BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V');
  rotor1.setPosition(charToIndex('A'));
  rotor2.setPosition(charToIndex('B'));
  rotor3.setPosition(charToIndex('C'));

  std::vector<Rotor> rotors;
  rotors.push_back(rotor1);
  rotors.push_back(rotor2);
  rotors.push_back(rotor3);

  Reflector reflector("YRUHQSLDPXNGOKMIEBFZCWVJAT");
  Plugboard plugboard;
  plugboard.addWire(charToIndex('A'), charToIndex('Z'));
  plugboard.addWire(charToIndex('B'), charToIndex('Y'));

  EnigmaMachine enigma(plugboard, rotors, reflector);

  std::string plaintext = "HELLOWORLDTHISISATEST";
  std::string ciphertext = enigma.processString(plaintext);

  // Reset
  rotor1.setPosition(charToIndex('A'));
  rotor2.setPosition(charToIndex('B'));
  rotor3.setPosition(charToIndex('C'));

  std::vector<Rotor> resetRotors;
  resetRotors.push_back(rotor1);
  resetRotors.push_back(rotor2);
  resetRotors.push_back(rotor3);

  EnigmaMachine enigmaDecrypt(plugboard, resetRotors, reflector);
  std::string decrypted = enigmaDecrypt.processString(ciphertext);

  REQUIRE(plaintext == decrypted);
}

TEST_CASE("Signal traces expose each encryption stage", "[enigma][trace]") {
  Rotor rotor1("EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q');
  Rotor rotor2("AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E');
  Rotor rotor3("BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V');
  std::vector<Rotor> rotors{rotor1, rotor2, rotor3};

  Plugboard plugboard;
  plugboard.addWire(charToIndex('A'), charToIndex('Z'));
  Reflector reflector("YRUHQSLDPXNGOKMIEBFZCWVJAT");

  EnigmaMachine tracedMachine(plugboard, rotors, reflector);
  EnigmaMachine referenceMachine(plugboard, rotors, reflector);
  const SignalTrace trace = tracedMachine.processCharWithTrace('A');

  REQUIRE(trace.input == 'A');
  REQUIRE(trace.plugboardForward == 'Z');
  REQUIRE(trace.rotorForward.size() == 3);
  REQUIRE(trace.rotorBackward.size() == 3);
  REQUIRE(trace.output == referenceMachine.processChar('A'));
}

TEST_CASE("Invalid wiring cannot create a broken rotor or reflector",
          "[enigma]") {
  CHECK_THROWS_AS(Rotor("AAAAAAAAAAAAAAAAAAAAAAAAAA", 'Q'),
                  std::invalid_argument);
  CHECK_THROWS_AS(Reflector("BCDEFGHIJKLMNOPQRSTUVWXYZA"),
                  std::invalid_argument);
}

TEST_CASE("Workbench edits replay the message from its configured start",
          "[workbench]") {
  workbench::Session edited;
  for (char letter : std::string("ENIGMA")) {
    REQUIRE(edited.typeLetter(letter));
  }
  edited.backspace();
  edited.adjustRotor(2, -1, 1);
  REQUIRE(edited.selectPlug(charToIndex('C')) ==
          workbench::PlugAction::Selected);
  REQUIRE(edited.selectPlug(charToIndex('A')) ==
          workbench::PlugAction::Connected);
  REQUIRE_FALSE(edited.isPlugConnected(charToIndex('Z')));

  workbench::Session fresh;
  fresh.adjustRotor(2, -1, 1);
  REQUIRE(fresh.selectPlug(charToIndex('A')) == workbench::PlugAction::Removed);
  REQUIRE(fresh.selectPlug(charToIndex('A')) ==
          workbench::PlugAction::Selected);
  REQUIRE(fresh.selectPlug(charToIndex('C')) ==
          workbench::PlugAction::Connected);
  for (char letter : std::string("ENIGM")) {
    REQUIRE(fresh.typeLetter(letter));
  }
  REQUIRE(edited.input() == fresh.input());
  REQUIRE(edited.output() == fresh.output());
  REQUIRE(edited.positionAt(2) == fresh.positionAt(2));
  REQUIRE(edited.trace()->input == 'M');
}

TEST_CASE("Pasting formats letters without altering how the machine steps",
          "[workbench]") {
  workbench::Session pasted;
  workbench::Session typed;
  REQUIRE(pasted.paste("Enig-ma! 123") == 6);
  for (const char letter : std::string("ENIGMA")) {
    REQUIRE(typed.typeLetter(letter));
  }
  CHECK(pasted.input() == typed.input());
  CHECK(pasted.output() == typed.output());
  CHECK(pasted.positionAt(2) == typed.positionAt(2));

  CHECK(pasted.paste(std::string(workbench::messageLimit, 'z')) ==
        workbench::messageLimit - 6);
  CHECK(pasted.input().size() == workbench::messageLimit);
  CHECK(pasted.paste("MORE") == 0);
}

TEST_CASE("Pasting can reuse the session's own tape", "[workbench]") {
  const std::string message = "THEQUICKBROWNFOXJUMPSOVERTHELAZYDOG";
  workbench::Session session;
  REQUIRE(session.paste(message) == message.size());

  SECTION("Plaintext") {
    REQUIRE(session.paste(session.input()) == message.size());
    CHECK(session.input() == message + message);
  }

  SECTION("Ciphertext") {
    const std::string ciphertext = session.output();
    REQUIRE(session.paste(session.output()) == ciphertext.size());
    CHECK(session.input() == message + ciphertext);
  }
}

TEST_CASE("Changing the rotor count rebuilds the entire tape and trace",
          "[workbench]") {
  workbench::Session session;
  REQUIRE(session.rotorCount() == 3);
  REQUIRE(session.paste("The signal follows every rotor") == 26);
  const auto plaintext = session.input();
  const auto threeRotorOutput = session.output();

  REQUIRE(session.setRotorCount(4));
  CHECK(session.trace()->rotorForward.size() == 4);
  CHECK(session.trace()->rotorBackward.size() == 4);
  const auto fourRotorOutput = session.output();
  CHECK(fourRotorOutput != threeRotorOutput);

  REQUIRE(session.setRotorCount(5));
  CHECK(session.trace()->rotorForward.size() == 5);
  CHECK(session.trace()->rotorBackward.size() == 5);
  CHECK(session.output() != fourRotorOutput);
  session.adjustRotor(4, 1, 2);
  const auto adjustedOutput = session.output();
  CHECK_FALSE(session.setRotorCount(6));
  CHECK_FALSE(session.setRotorCount(2));
  CHECK(session.rotorCount() == 5);
  CHECK(session.output() == adjustedOutput);

  REQUIRE(session.setRotorCount(3));
  CHECK(session.output() == threeRotorOutput);
  REQUIRE(session.setRotorCount(5));
  CHECK(session.startAt(4) == 1);
  CHECK(session.ringAt(4) == 2);
  CHECK(session.output() == adjustedOutput);
  CHECK(session.input() == plaintext);

  session.reset();
  CHECK(session.rotorCount() == 3);
  CHECK(session.input().empty());
}

TEST_CASE("Four and five rotor configurations can decipher their output",
          "[workbench]") {
  const std::string message = "EVERYROTORSTEPCOUNTSWHENDECIPHERINGMESSAGES";
  for (std::size_t count = 4; count <= workbench::maximumRotorCount; ++count) {
    CAPTURE(count);
    workbench::Session sender;
    workbench::Session receiver;
    REQUIRE(sender.setRotorCount(count));
    REQUIRE(receiver.setRotorCount(count));
    sender.adjustRotor(count - 1, -1, 2);
    receiver.adjustRotor(count - 1, -1, 2);
    REQUIRE(sender.paste(message) == message.size());
    REQUIRE(receiver.paste(sender.output()) == message.size());
    CHECK(receiver.output() == message);
  }
}

#ifndef ENIGMA_CORE_ONLY

TEST_CASE("Geometry math and collision hit tests", "[workbench][geometry]") {
  using namespace workbench;

  const Rectangle box{10.0F, 20.0F, 100.0F, 50.0F};
  CHECK(right(box) == 110.0F);
  CHECK(bottom(box) == 70.0F);
  CHECK(center(box).x == 60.0F);
  CHECK(center(box).y == 45.0F);

  const Rectangle inset = insetBy(box, 5.0F);
  CHECK(inset.x == 15.0F);
  CHECK(inset.y == 25.0F);
  CHECK(inset.width == 90.0F);
  CHECK(inset.height == 40.0F);

  CHECK(checkCollisionPointRec({50.0F, 40.0F}, box));
  CHECK(checkCollisionPointRec({10.0F, 20.0F}, box));   // boundary
  CHECK(checkCollisionPointRec({110.0F, 70.0F}, box));  // boundary
  CHECK_FALSE(checkCollisionPointRec({9.9F, 40.0F}, box));
  CHECK_FALSE(checkCollisionPointRec({110.1F, 40.0F}, box));

  const Vector2 circleCenter{100.0F, 100.0F};
  constexpr float radius = 25.0F;
  CHECK(checkCollisionPointCircle({100.0F, 100.0F}, circleCenter, radius));
  CHECK(checkCollisionPointCircle({125.0F, 100.0F}, circleCenter, radius)); // on edge
  CHECK_FALSE(checkCollisionPointCircle({126.0F, 100.0F}, circleCenter, radius));

  CHECK(srgbToLinearChannel(0.0F) == 0.0F);
  CHECK(srgbToLinearChannel(1.0F) == 1.0F);
  const auto bgLin = colorToLinear(background);
  CHECK(bgLin[3] == 1.0F);
  CHECK(bgLin[0] > 0.0F);
}

TEST_CASE("Responsive layout preserves minimum bounds and rotor counts",
          "[workbench][layout]") {
  using namespace workbench;

  SECTION("Minimum window size") {
    const Layout minLayout = makeLayout(minimumWidth, minimumHeight, 3);
    CHECK(minLayout.content.width > 0.0F);
    CHECK(minLayout.content.height > 0.0F);
    CHECK(minLayout.keyboard.width > 0.0F);
    CHECK(minLayout.machine.width > 0.0F);
    CHECK(minLayout.keys.size() == 26);
    CHECK(minLayout.plugCenters.size() == 26);
    CHECK(minLayout.plugRadius > 0.0F);
  }

  SECTION("Rotor counts 3, 4, and 5 adjust row bounds") {
    for (std::size_t count = 3; count <= 5; ++count) {
      const Layout layout = makeLayout(1320, 840, count);
      for (std::size_t i = 0; i < count; ++i) {
        CHECK(layout.rotorRows[i].bounds.width > 0.0F);
        CHECK(layout.rotorRows[i].bounds.height > 0.0F);
        if (i > 0) {
          CHECK(layout.rotorRows[i].bounds.y >
                layout.rotorRows[i - 1].bounds.y);
        }
      }
    }
  }

  SECTION("Rotor count is clamped to supported range") {
    const Layout clampedMin = makeLayout(1320, 840, 1);
    const Layout clampedMax = makeLayout(1320, 840, 10);
    const Layout three = makeLayout(1320, 840, 3);
    const Layout five = makeLayout(1320, 840, 5);
    CHECK(clampedMin.rotorRows[0].bounds == three.rotorRows[0].bounds);
    CHECK(clampedMax.rotorRows[4].bounds == five.rotorRows[4].bounds);
  }
}

TEST_CASE("Input event pipeline processes keyboard, mouse, and modal priority",
          "[workbench][input]") {
  using namespace workbench;

  Session session;
  Layout layout = makeLayout(1320, 840, session.rotorCount());
  InterfaceState state;
  bool shouldClose = false;
  std::string clipboardStorage;

  PlatformContext platform{
      .setClipboard = [&](std::string_view text) { clipboardStorage = text; },
      .getClipboard = [&]() -> std::string { return clipboardStorage; }};

  SECTION("Typing converts lowercase to uppercase and steps cipher") {
    InputEvent e{};
    e.type = EventType::Char;
    e.codepoint = 'e';
    CHECK(handleEvent(session, layout, state, e, platform, shouldClose));
    CHECK(session.input() == "E");

    e.codepoint = 'n';
    CHECK(handleEvent(session, layout, state, e, platform, shouldClose));
    CHECK(session.input() == "EN");

    // Non-letters ignored
    e.codepoint = '1';
    CHECK_FALSE(handleEvent(session, layout, state, e, platform, shouldClose));
    CHECK(session.input() == "EN");

    // Backspace
    InputEvent backspaceEvent{};
    backspaceEvent.type = EventType::Key;
    backspaceEvent.key = 259; // KeyBackspace
    backspaceEvent.action = 1;
    CHECK(handleEvent(session, layout, state, backspaceEvent, platform, shouldClose));
    CHECK(session.input() == "E");
  }

  SECTION("Shortcuts Ctrl+C and Ctrl+V") {
    session.typeLetter('A');
    session.typeLetter('B');
    const std::string out = session.output();

    // Ctrl+C
    InputEvent ctrlC{};
    ctrlC.type = EventType::Key;
    ctrlC.key = 67; // KeyC
    ctrlC.mods = 2; // ModControl
    ctrlC.action = 1;
    CHECK(handleEvent(session, layout, state, ctrlC, platform, shouldClose));
    CHECK(clipboardStorage == out);
    CHECK(state.status.current() == "Ciphertext copied");

    // Clear button via click
    InputEvent clickClear{};
    clickClear.type = EventType::MouseButton;
    clickClear.button = 0;
    clickClear.action = 1;
    clickClear.cursor = center(layout.clear);
    CHECK(handleEvent(session, layout, state, clickClear, platform, shouldClose));
    CHECK(session.input().empty());
    CHECK(state.status.current() == "Message cleared");

    // Ctrl+V
    InputEvent ctrlV{};
    ctrlV.type = EventType::Key;
    ctrlV.key = 86; // KeyV
    ctrlV.mods = 2; // ModControl
    ctrlV.action = 1;
    CHECK(handleEvent(session, layout, state, ctrlV, platform, shouldClose));
    CHECK(session.input() == out);
  }

  SECTION("Modal priority: Escape closes photo modal first, then exits") {
    // Open photo modal via click
    InputEvent clickPhoto{};
    clickPhoto.type = EventType::MouseButton;
    clickPhoto.button = 0;
    clickPhoto.action = 1;
    clickPhoto.cursor = center(layout.openPhoto);
    CHECK(handleEvent(session, layout, state, clickPhoto, platform, shouldClose));
    CHECK(state.photoOpen);
    CHECK_FALSE(shouldClose);

    // Escape while photo is open
    InputEvent esc{};
    esc.type = EventType::Key;
    esc.key = 256; // KeyEscape
    esc.action = 1;
    CHECK(handleEvent(session, layout, state, esc, platform, shouldClose));
    CHECK_FALSE(state.photoOpen);
    CHECK_FALSE(shouldClose);

    // Escape while photo is closed exits window
    CHECK(handleEvent(session, layout, state, esc, platform, shouldClose));
    CHECK(shouldClose);
  }

  SECTION("Rotor count stepper click recomputes layout") {
    InputEvent clickInc{};
    clickInc.type = EventType::MouseButton;
    clickInc.button = 0;
    clickInc.action = 1;
    clickInc.cursor = center(layout.rotorCountControl.increase);
    CHECK(handleEvent(session, layout, state, clickInc, platform, shouldClose));
    CHECK(session.rotorCount() == 4);
  }
}

TEST_CASE("Font atlas loads TTF and measures text accurately",
          "[workbench][font]") {
  using namespace workbench;

  FontAtlas atlas;
  const std::string fontPath = "assets/Montserrat-Medium.ttf";
  REQUIRE(atlas.load(fontPath, 48, controlGlyphs));
  CHECK(atlas.baseSize() == 48);
  CHECK(atlas.ascent() > 0.0F);
  CHECK(atlas.lineHeight() > 0.0F);
  CHECK(atlas.atlasWidth() == 1024);
  CHECK(atlas.atlasHeight() == 1024);

  const auto *glyphM = atlas.findGlyph('M');
  REQUIRE(glyphM != nullptr);
  CHECK(glyphM->advanceX > 0.0F);

  const auto *glyphArrow = atlas.findGlyph(0x2192); // Arrow glyph
  REQUIRE(glyphArrow != nullptr);

  const auto extent = atlas.measure("Enigma Workbench", 24.0F);
  CHECK(extent.x > 0.0F);
  CHECK(extent.y == 24.0F);

  const float advance = atlas.lineAdvance(24.0F);
  CHECK(advance > extent.y);
  CHECK(atlas.measure("M\nM", 24.0F).y == extent.y + advance);

  DrawList lines;
  lines.addText(atlas, "M\nM", {0, 0}, 24.0F);
  REQUIRE(lines.vertices().size() == 8);
  CHECK_THAT(lines.vertices()[4].pos[1] - lines.vertices()[0].pos[1],
             Catch::Matchers::WithinAbs(advance, 0.001));

  const auto emptyExtent = atlas.measure("", 24.0F);
  CHECK(emptyExtent.x == 0.0F);
  CHECK(emptyExtent.y == 0.0F);
}

TEST_CASE("ImageDecoder decodes museum JPEG and handles corrupted files",
          "[workbench][image]") {
  using namespace workbench;

  const auto valid = decodeJpeg("assets/Enigma-Imperial-War-Museum.jpg");
  REQUIRE(valid.valid);
  CHECK(valid.width == 1414);
  CHECK(valid.height == 1886);
  CHECK(valid.rgba.size() == 1414 * 1886 * 4);

  const auto missing = decodeJpeg("assets/non_existent_file.jpg");
  CHECK_FALSE(missing.valid);
}

TEST_CASE("2D DrawList batches primitives and preserves order",
          "[workbench][draw]") {
  using namespace workbench;

  DrawList dl;
  dl.addRectangle({0.0F, 0.0F, 100.0F, 50.0F}, panel);
  dl.addBox({10.0F, 10.0F, 40.0F, 30.0F}, well, border);
  dl.addLine({0.0F, 0.0F}, {50.0F, 50.0F}, 2.0F, wireLit);
  dl.addCircle({25.0F, 25.0F}, 10.0F, amber);

  CHECK(dl.vertices().size() > 0);
  CHECK(dl.indices().size() > 0);
  // All solid primitives should merge into 1 batch
  REQUIRE(dl.batches().size() == 1);
  CHECK(dl.batches()[0].pipeline == PipelineType::Solid);

  // Adding sampled primitive splits the batch
  dl.addTexturedQuad(42, {0.0F, 0.0F, 50.0F, 50.0F},
                     {0.0F, 0.0F, 50.0F, 50.0F}, white, {100.0F, 100.0F});
  REQUIRE(dl.batches().size() == 2);
  CHECK(dl.batches()[1].pipeline == PipelineType::Sampled);
  CHECK(dl.batches()[1].textureId == 42);

  // Full workbench scene generation
  Session session;
  InterfaceState state;
  Assets assets;
  assets.ui.load("assets/Montserrat-Medium.ttf", 48, controlGlyphs);
  assets.mono.load("assets/JetBrainsMono-Medium.ttf", 44, controlGlyphs);
  assets.title.load("assets/InstrumentSerif-Regular.ttf", 64);
  Layout layout = makeLayout(1320, 840, 3);

  dl.clear();
  drawWorkbench(session, layout, assets, state, {0.0F, 0.0F}, dl, 1320, 840);
  CHECK(dl.batches().size() > 0);
  CHECK(dl.vertices().size() > 100);

  // With photo modal open
  state.photoOpen = true;
  dl.clear();
  drawWorkbench(session, layout, assets, state, {0.0F, 0.0F}, dl, 1320, 840);
  CHECK(dl.batches().size() > 0);
}

TEST_CASE("VulkanRenderer cleanly fails initialization without a window",
          "[workbench][vulkan]") {
  using namespace workbench;

  VulkanRenderer renderer;
  CHECK_FALSE(renderer.init(nullptr));
  CHECK_FALSE(renderer.isInitialized());
  renderer.shutdown(); // Idempotent shutdown
}

TEST_CASE("Font fallback loads alternate face when primary is missing",
          "[workbench][font]") {
  using namespace workbench;

  FontAtlas atlas;
  const std::string root = resolveAssetDirectory() + "/";
  // "NonExistentFont.ttf" does not exist, so loadFontWithFallback should fall back to Montserrat-Medium.ttf
  const bool loaded =
      loadFontWithFallback(atlas, root, "NonExistentFont.ttf", 32);
  REQUIRE(loaded);
  CHECK(atlas.baseSize() == 32);
  CHECK(atlas.findGlyph('A') != nullptr);

  // If no faces exist in invalid directory, it should fail cleanly
  FontAtlas missingAtlas;
  const bool missingLoaded =
      loadFontWithFallback(missingAtlas, "invalid_non_existent_dir/",
                           "NoSuchFont.ttf", 32);
  CHECK_FALSE(missingLoaded);
}

TEST_CASE(
    "Asset loading initializes fonts and photo with graceful error handling",
    "[workbench][assets]") {
  using namespace workbench;

  VulkanRenderer uninitializedRenderer;
  // Should successfully rasterize fonts on CPU even if GPU renderer is not initialized
  const Assets assets = loadAssets(uninitializedRenderer);
  CHECK(assets.fontsLoaded);
  CHECK(assets.title.baseSize() == 64);
  CHECK(assets.ui.baseSize() == 48);
  CHECK(assets.mono.baseSize() == 44);
  // Without GPU device, textures remain 0
  CHECK(assets.title.textureId() == 0);
  CHECK(assets.ui.textureId() == 0);
  CHECK(assets.mono.textureId() == 0);
}

#endif // ENIGMA_CORE_ONLY
