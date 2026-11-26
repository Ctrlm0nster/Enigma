#include "Input.hpp"

#include "Charactermap.hpp"
#include "Layout.hpp"

namespace workbench {
namespace {

void copyOutput(const Session &session, Status &status) {
  if (!session.output().empty()) {
    SetClipboardText(session.output().c_str());
    status.show("Ciphertext copied");
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

void handleClick(Session &session, const Layout &layout, Status &status,
                 Vector2 cursor) {
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
    session.clearPlugboard();
    status.show("Plugboard cleared");
    return;
  }

  for (std::size_t i = 0; i < layout.rotorRows.size(); ++i) {
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
    if (CheckCollisionPointCircle(cursor, layout.keyCenters[idx(i)],
                                  layout.keyRadius)) {
      typeLetter(session, status, indexToChar(i));
      return;
    }
  }
}

} // namespace

void Status::show(std::string_view value) {
  message = value;
  expiresAt = GetTime() + 3.0;
}

std::string_view Status::current() const {
  return GetTime() < expiresAt ? message : std::string_view{};
}

void handleInput(Session &session, const Layout &layout, Status &status) {
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    handleClick(session, layout, status, GetMousePosition());
  }
  if (IsKeyPressed(KEY_BACKSPACE)) {
    session.backspace();
  }
  const bool control =
      IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  if (control && IsKeyPressed(KEY_C)) {
    copyOutput(session, status);
  }
  for (int letter = GetCharPressed(); letter > 0; letter = GetCharPressed()) {
    if (letter >= 'a' && letter <= 'z') {
      letter += 'A' - 'a';
    }
    if (!control && letter >= 'A' && letter <= 'Z') {
      typeLetter(session, status, static_cast<char>(letter));
    }
  }
}

} // namespace workbench
