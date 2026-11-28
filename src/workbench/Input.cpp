#include "Input.hpp"

#include "AudioEngine.hpp"
#include "Charactermap.hpp"
#include "Layout.hpp"
#include "Session.hpp"

namespace workbench {
namespace {

constexpr int KeyEscape = 256;
constexpr int KeyBackspace = 259;
constexpr int KeyC = 67;
constexpr int KeyV = 86;
constexpr unsigned ModControl = 0x0002U;
constexpr int ActionRelease = 0;
constexpr int ActionPress = 1;
constexpr int ActionRepeat = 2;
constexpr int MouseButtonLeft = 0;

void copyOutput(const Session &session, Status &status,
                const PlatformContext &platform) {
  if (!session.output().empty()) {
    const bool copied = platform.setClipboard && platform.setClipboard(session.output());
    status.show(copied ? "Ciphertext copied" : "Could not copy ciphertext");
  }
}

void pasteInput(Session &session, Status &status,
                const PlatformContext &platform, AudioEngine *audio = nullptr) {
  if (session.input().size() >= messageLimit) {
    status.show("Message tape is full");
    return;
  }
  const std::string text = platform.getClipboard ? platform.getClipboard() : "";
  const auto count = !text.empty() ? session.paste(text) : 0;
  if (count != 0) {
    if (audio) {
      audio->play(SoundEffect::KeyClick);
      audio->play(SoundEffect::RotorStep);
    }
    const std::string detail = "Pasted " + std::to_string(count) +
                               (count == 1 ? " letter" : " letters");
    status.show(detail);
  } else {
    status.show("No A–Z letters in the clipboard");
  }
}

void typeLetter(Session &session, Status &status, char letter, AudioEngine *audio = nullptr) {
  if (audio) {
    audio->play(SoundEffect::KeyClick);
    audio->play(SoundEffect::RotorStep);
  }
  if (!session.typeLetter(letter)) {
    status.show("Message limit reached");
  }
}

void handlePlug(Session &session, Status &status, int socket, AudioEngine *audio = nullptr) {
  if (audio) {
    audio->play(SoundEffect::PlugSnap);
  }
  switch (session.selectPlug(socket)) {
  case PlugAction::Selected:
  case PlugAction::Cancelled:
    break;
  case PlugAction::Removed:
    status.show("Plugboard lead removed");
    break;
  case PlugAction::Connected:
    status.show("Plugboard lead connected");
    break;
  case PlugAction::LimitReached:
    status.show("The plugboard supports up to 10 leads");
    break;
  }
}

int stepperDelta(const Stepper &control, Vector2 cursor) {
  if (checkCollisionPointRec(cursor, control.decrease)) {
    return -1;
  }
  return checkCollisionPointRec(cursor, control.increase) ? 1 : 0;
}

void handleClick(Session &session, Layout &layout, InterfaceState &state,
                 Vector2 cursor, const PlatformContext &platform, AudioEngine *audio) {
  auto &status = state.status;
  if (state.photoOpen) {
    if (checkCollisionPointRec(cursor, layout.closePhoto) ||
        !checkCollisionPointRec(cursor, layout.photoViewer)) {
      state.photoOpen = false;
    }
    return;
  }

  if (state.presetsOpen) {
    const auto presets = enigma::historicalPresets();
    if (checkCollisionPointRec(cursor, layout.presetsMenu)) {
      for (std::size_t i = 0; i < presets.size(); ++i) {
        if (checkCollisionPointRec(cursor, layout.presetRow(i))) {
          session.loadPreset(i);
          layout = makeLayout(layout.screenWidth, layout.screenHeight, session.rotorCount());
          state.presetsOpen = false;
          if (audio) {
            audio->play(SoundEffect::RotorStep);
          }
          status.show(std::string("Loaded ") + std::string(presets[i].name));
          return;
        }
      }
    }
    state.presetsOpen = false;
    return;
  }

  if (checkCollisionPointRec(cursor, layout.soundToggle)) {
    if (audio) {
      audio->setMuted(!audio->isMuted());
      status.show(audio->isMuted() ? "Sound muted" : "Sound enabled");
    }
    return;
  }

  if (checkCollisionPointRec(cursor, layout.presetsButton)) {
    state.presetsOpen = !state.presetsOpen;
    return;
  }

  if (checkCollisionPointRec(cursor, layout.photo) ||
      checkCollisionPointRec(cursor, layout.openPhoto)) {
    state.photoOpen = true;
    return;
  }
  if (checkCollisionPointRec(cursor, layout.paste)) {
    pasteInput(session, status, platform, audio);
    return;
  }
  if (checkCollisionPointRec(cursor, layout.copy)) {
    copyOutput(session, status, platform);
    return;
  }
  if (checkCollisionPointRec(cursor, layout.clear)) {
    if (!session.input().empty()) {
      session.clearMessage();
      status.show("Message cleared");
    }
    return;
  }
  if (checkCollisionPointRec(cursor, layout.reset)) {
    session.reset();
    status.show("Machine reset to defaults");
    layout = makeLayout(layout.screenWidth, layout.screenHeight,
                        session.rotorCount());
    return;
  }
  if (checkCollisionPointRec(cursor, layout.clearPlugboard)) {
    if (!session.plugPairs().empty() || session.selectedPlug()) {
      session.clearPlugboard();
      status.show("Plugboard cleared");
    }
    return;
  }

  if (checkCollisionPointRec(cursor, layout.reflectorButton)) {
    const int nextRefl = (session.reflectorIndex() + 1) % static_cast<int>(enigma::reflectorNames.size());
    session.setReflectorIndex(nextRefl);
    if (audio) {
      audio->play(SoundEffect::RotorStep);
    }
    status.show(std::string("Reflector changed to ") + enigma::reflectorNames[nextRefl]);
    return;
  }

  if (const int delta = stepperDelta(layout.rotorCountControl, cursor);
      delta != 0) {
    const auto count = static_cast<int>(session.rotorCount()) + delta;
    if (session.setRotorCount(static_cast<std::size_t>(count))) {
      if (audio) {
        audio->play(SoundEffect::RotorStep);
      }
      status.show("Using " + std::to_string(count) + " rotors");
      layout = makeLayout(layout.screenWidth, layout.screenHeight,
                          session.rotorCount());
    }
    return;
  }

  for (std::size_t i = 0; i < session.rotorCount(); ++i) {
    const auto &row = layout.rotorRows[i];
    if (checkCollisionPointRec(cursor, row.modelButton)) {
      const int nextType = (session.rotorTypeAt(i) + 1) % static_cast<int>(enigma::rotorNames.size());
      session.setRotorType(i, nextType);
      if (audio) {
        audio->play(SoundEffect::RotorStep);
      }
      status.show(std::string("Rotor ") + std::to_string(i + 1) + " changed to " + enigma::rotorNames[nextType]);
      return;
    }
    const int startDelta = stepperDelta(row.start, cursor);
    const int ringDelta = stepperDelta(row.ring, cursor);
    if (startDelta != 0 || ringDelta != 0) {
      session.adjustRotor(i, startDelta, ringDelta);
      if (audio) {
        audio->play(SoundEffect::RotorStep);
      }
      status.show("Rotor settings updated");
      return;
    }
  }

  for (int i = 0; i < 26; ++i) {
    if (checkCollisionPointCircle(cursor, layout.plugCenters[idx(i)],
                                  layout.plugRadius)) {
      handlePlug(session, status, i, audio);
      return;
    }
    if (checkCollisionPointRec(cursor, layout.keys[idx(i)])) {
      state.pressedKey = indexToChar(i);
      typeLetter(session, status, indexToChar(i), audio);
      return;
    }
  }
}

} // namespace

void Status::show(std::string_view value) {
  message = value;
  expiresAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(3500);
}

std::string_view Status::current() const {
  return !isExpired() ? std::string_view{message} : std::string_view{};
}

bool Status::isExpired() const {
  return std::chrono::steady_clock::now() >= expiresAt;
}

double Status::secondsRemaining() const {
  if (isExpired()) {
    return 0.0;
  }
  const auto diff = expiresAt - std::chrono::steady_clock::now();
  return std::chrono::duration<double>(diff).count();
}

bool handleEvent(Session &session, Layout &layout, InterfaceState &state,
                 const InputEvent &event, const PlatformContext &platform,
                 bool &shouldClose, AudioEngine *audio) {
  switch (event.type) {
  case EventType::Key: {
    if (event.action == ActionRelease) {
      state.pressedKey.reset();
      return true;
    }
    if (event.action == ActionPress || event.action == ActionRepeat) {
      if (event.key == KeyEscape) {
        if (state.photoOpen) {
          state.photoOpen = false;
        } else if (state.presetsOpen) {
          state.presetsOpen = false;
        } else {
          shouldClose = true;
        }
        return true;
      }
      if (state.presetsOpen || state.photoOpen) {
        return false;
      }
      if (event.key == KeyBackspace) {
        session.backspace();
        return true;
      }
      const bool control = (static_cast<unsigned>(event.mods) & ModControl) != 0;
      if (control && (event.key == KeyC || event.key == 'c')) {
        copyOutput(session, state.status, platform);
        return true;
      }
      if (control && (event.key == KeyV || event.key == 'v')) {
        pasteInput(session, state.status, platform, audio);
        return true;
      }
    }
    return false;
  }
  case EventType::Char: {
    const bool control = (static_cast<unsigned>(event.mods) & ModControl) != 0;
    if (control || state.presetsOpen || state.photoOpen) {
      return false;
    }
    uint32_t cp = event.codepoint;
    if (cp >= 'a' && cp <= 'z') {
      cp = cp - 'a' + 'A';
    }
    if (cp >= 'A' && cp <= 'Z') {
      state.pressedKey = static_cast<char>(cp);
      typeLetter(session, state.status, static_cast<char>(cp), audio);
      return true;
    }
    return false;
  }
  case EventType::MouseButton: {
    if (event.button == MouseButtonLeft) {
      if (event.action == ActionPress) {
        handleClick(session, layout, state, event.cursor, platform, audio);
        return true;
      }
      if (event.action == ActionRelease) {
        state.pressedKey.reset();
        return true;
      }
    }
    return false;
  }
  case EventType::CursorPos: {
    return true; // Cursor moved, trigger redraw for hover states
  }
  case EventType::WindowResize: {
    layout = makeLayout(event.width, event.height, session.rotorCount());
    return true;
  }
  case EventType::WindowClose: {
    shouldClose = true;
    return true;
  }
  }
  return false;
}

} // namespace workbench
