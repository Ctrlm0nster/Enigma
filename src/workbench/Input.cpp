#include "Input.hpp"

#include "Charactermap.hpp"
#include "Layout.hpp"
#include "Session.hpp"
#include "raylib.h"

namespace workbench {
namespace {

void copyOutput(const Session &session, Status &status) {
  if (!session.output().empty()) {
    SetClipboardText(session.output().c_str());
    status.show("Ciphertext copied");
  }
}

void pasteInput(Session &session, Status &status) {
  if (session.input().size() >= messageLimit) {
    status.show("Message tape is full");
    return;
  }
  const char *clipboard = GetClipboardText();
  const auto count = clipboard ? session.paste(clipboard) : 0;
  if (count != 0) {
    const std::string detail = "Pasted " + std::to_string(count) +
                               (count == 1 ? " letter" : " letters");
    status.show(detail);
  } else {
    status.show("No A–Z letters in the clipboard");
  }
}

void typeLetter(Session &session, Status &status, char letter) {
  if (!session.typeLetter(letter)) {
    status.show("Message limit reached");
  }
}

void handlePlug(Session &session, Status &status, int socket) {
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
  if (CheckCollisionPointRec(cursor, control.decrease)) {
    return -1;
  }
  return CheckCollisionPointRec(cursor, control.increase) ? 1 : 0;
}

void handleClick(Session &session, const Layout &layout, InterfaceState &state,
                 Vector2 cursor) {
  auto &status = state.status;
  if (CheckCollisionPointRec(cursor, layout.photo) ||
      CheckCollisionPointRec(cursor, layout.openPhoto)) {
    state.photoOpen = true;
    return;
  }
  if (CheckCollisionPointRec(cursor, layout.paste)) {
    pasteInput(session, status);
    return;
  }
  if (CheckCollisionPointRec(cursor, layout.copy)) {
    copyOutput(session, status);
    return;
  }
  if (CheckCollisionPointRec(cursor, layout.clear)) {
    if (!session.input().empty()) {
      session.clearMessage();
      status.show("Message cleared");
    }
    return;
  }
  if (CheckCollisionPointRec(cursor, layout.reset)) {
    session.reset();
    status.show("Machine reset to defaults");
    return;
  }
  if (CheckCollisionPointRec(cursor, layout.clearPlugboard)) {
    if (!session.plugPairs().empty() || session.selectedPlug()) {
      session.clearPlugboard();
      status.show("Plugboard cleared");
    }
    return;
  }

  if (const int delta = stepperDelta(layout.rotorCountControl, cursor);
      delta != 0) {
    const auto count = static_cast<int>(session.rotorCount()) + delta;
    if (session.setRotorCount(static_cast<std::size_t>(count))) {
      status.show("Using " + std::to_string(count) + " rotors");
    }
    return;
  }

  for (std::size_t i = 0; i < session.rotorCount(); ++i) {
    const auto &row = layout.rotorRows[i];
    const int startDelta = stepperDelta(row.start, cursor);
    const int ringDelta = stepperDelta(row.ring, cursor);
    if (startDelta != 0 || ringDelta != 0) {
      session.adjustRotor(i, startDelta, ringDelta);
      status.show("Rotor settings updated");
      return;
    }
  }

  for (int i = 0; i < 26; ++i) {
    if (CheckCollisionPointCircle(cursor, layout.plugCenters[idx(i)],
                                  layout.plugRadius)) {
      handlePlug(session, status, i);
      return;
    }
    if (CheckCollisionPointRec(cursor, layout.keys[idx(i)])) {
      typeLetter(session, status, indexToChar(i));
      return;
    }
  }
}

} // namespace

void Status::show(std::string_view value) {
  message = value;
  expiresAt = GetTime() + 3.5;
}

std::string_view Status::current() const {
  return GetTime() < expiresAt ? std::string_view{message} : std::string_view{};
}

void handleInput(Session &session, const Layout &layout,
                 InterfaceState &state) {
  if (state.photoOpen) {
    const Vector2 cursor = GetMousePosition();
    const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (IsKeyPressed(KEY_ESCAPE) ||
        (clicked && (CheckCollisionPointRec(cursor, layout.closePhoto) ||
                     !CheckCollisionPointRec(cursor, layout.photoViewer)))) {
      state.photoOpen = false;
    }
    return;
  }
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    handleClick(session, layout, state, GetMousePosition());
    if (state.photoOpen) {
      return;
    }
  }
  if (IsKeyPressed(KEY_BACKSPACE)) {
    session.backspace();
  }
  const bool control =
      IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  if (control && IsKeyPressed(KEY_C)) {
    copyOutput(session, state.status);
  }
  if (control && IsKeyPressed(KEY_V)) {
    pasteInput(session, state.status);
  }
  for (int letter = GetCharPressed(); letter > 0; letter = GetCharPressed()) {
    if (letter >= 'a' && letter <= 'z') {
      letter += 'A' - 'a';
    }
    if (!control && letter >= 'A' && letter <= 'Z') {
      typeLetter(session, state.status, static_cast<char>(letter));
    }
  }
}

} // namespace workbench
