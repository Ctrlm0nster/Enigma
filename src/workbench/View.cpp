#include "View.hpp"

#include "Charactermap.hpp"
#include "Layout.hpp"

#include <array>
#include <string>

namespace workbench {
namespace {

constexpr Color backdrop{18, 24, 22, 255};
constexpr Color surface{28, 36, 32, 255};
constexpr Color raised{38, 48, 43, 255};
constexpr Color hovered{52, 66, 59, 255};
constexpr Color divider{67, 82, 74, 255};
constexpr Color outline{119, 135, 126, 255};
constexpr Color ink{248, 246, 233, 255};
constexpr Color secondary{199, 211, 202, 255};
constexpr Color gold{244, 200, 110, 255};
constexpr Color goldSurface{59, 49, 30, 255};
constexpr Color mint{159, 226, 183, 255};
constexpr Color mintSurface{37, 69, 50, 255};

Vector2 center(Rectangle box) {
  return {box.x + box.width / 2, box.y + box.height / 2};
}

void card(Rectangle bounds) {
  DrawRectangleRounded(bounds, 0.075F, 8, surface);
  DrawRectangleRoundedLines(bounds, 0.075F, 8, 1, divider);
}

void write(Font font, const std::string &value, Vector2 origin, float size,
           Color color = ink) {
  DrawTextEx(font, value.c_str(), origin, size, 0.5F, color);
}

Vector2 measure(Font font, const std::string &value, float size) {
  return MeasureTextEx(font, value.c_str(), size, 0.5F);
}

void centerText(Font font, const std::string &value, Vector2 point, float size,
                Color color = ink) {
  const Vector2 extent = measure(font, value, size);
  write(font, value, {point.x - extent.x / 2, point.y - extent.y / 2}, size,
        color);
}

void rightText(Font font, const std::string &value, float right, float y,
               float size, Color color) {
  write(font, value, {right - measure(font, value, size).x, y}, size, color);
}

void button(Font font, Rectangle bounds, const std::string &caption,
            Vector2 cursor, bool enabled = true) {
  const bool over = enabled && CheckCollisionPointRec(cursor, bounds);
  DrawRectangleRounded(bounds, 0.26F, 8, over ? hovered : raised);
  const Color edge = over ? gold : (enabled ? outline : divider);
  DrawRectangleRoundedLines(bounds, 0.26F, 8, 1, edge);
  centerText(font, caption, center(bounds), 15, enabled ? ink : secondary);
}

std::string grouped(const std::string &message) {
  std::string result;
  result.reserve(message.size() + message.size() / 5);
  for (std::size_t i = 0; i < message.size(); ++i) {
    if (i != 0 && i % 5 == 0) {
      result.push_back(' ');
    }
    result.push_back(message[i]);
  }
  return result;
}

std::string visibleMessage(Font font, const std::string &message, float width) {
  constexpr float size = 23;
  const std::string display = grouped(message);
  if (measure(font, display, size).x <= width) {
    return display;
  }
  for (auto space = display.find(' '); space != std::string::npos;
       space = display.find(' ', space + 1)) {
    const std::string tail = "... " + display.substr(space + 1);
    if (measure(font, tail, size).x <= width) {
      return tail;
    }
  }
  return "...";
}

std::string letterAt(int index) { return std::string(1, indexToChar(index)); }

void drawMessage(const Session &session, const Layout &layout,
                 const Fonts &fonts, Vector2 cursor) {
  const auto &bounds = layout.message;
  const float x = bounds.x + 18;
  const float y = bounds.y;
  card(bounds);
  write(fonts.ui, "Message", {x, y + 16}, 23);
  button(fonts.ui, layout.copy, "Copy output", cursor,
         !session.output().empty());
  button(fonts.ui, layout.clear, "Clear", cursor, !session.input().empty());

  write(fonts.ui, "Plaintext", {x, y + 53}, 15, secondary);
  if (session.input().empty()) {
    write(fonts.ui, "Type A–Z or click a key", {x, y + 73}, 18, secondary);
  } else {
    write(fonts.mono,
          visibleMessage(fonts.mono, session.input(), bounds.width - 36),
          {x, y + 70}, 23);
  }

  DrawLineEx({x, y + 106}, {bounds.x + bounds.width - 18, y + 106}, 1, divider);
  write(fonts.ui, "Ciphertext", {x, y + 116}, 15, secondary);
  if (session.output().empty()) {
    write(fonts.ui, "Waiting for a keypress", {x, y + 136}, 18, secondary);
  } else {
    write(fonts.mono,
          visibleMessage(fonts.mono, session.output(), bounds.width - 36),
          {x, y + 133}, 23, gold);
  }
}

void drawSignal(const Session &session, Rectangle bounds, const Fonts &fonts) {
  card(bounds);
  write(fonts.ui, "Signal path", {bounds.x + 18, bounds.y + 15}, 23);
  rightText(fonts.ui, "Last keypress", bounds.x + bounds.width - 18,
            bounds.y + 21, 15, secondary);

  const std::array<std::string, 7> labels{
      "Key",        "Plugboard", "Rotors in", "Reflector",
      "Rotors out", "Plugboard", "Lamp"};
  std::array<char, 7> letters{'-', '-', '-', '-', '-', '-', '-'};
  if (const auto &trace = session.trace()) {
    letters = {trace->input,
               trace->plugboardForward,
               trace->rotorForward.back(),
               trace->reflected,
               trace->rotorBackward.back(),
               trace->output,
               trace->output};
  }

  constexpr float gap = 12;
  const float width = (bounds.width - 36 - gap * 6) / 7;
  const float top = bounds.y + 49;
  for (std::size_t i = 0; i < labels.size(); ++i) {
    const float x = bounds.x + 18 + static_cast<float>(i) * (width + gap);
    const bool lamp = i == labels.size() - 1 && session.trace().has_value();
    const Rectangle stage{x, top, width, 61};
    DrawRectangleRounded(stage, 0.2F, 8, lamp ? goldSurface : raised);
    DrawRectangleRoundedLines(stage, 0.2F, 8, 1, lamp ? gold : divider);
    if (i != 0) {
      const float y = top + 31;
      DrawLineEx({x - 10, y}, {x - 3, y}, 1, outline);
      DrawLineEx({x - 7, y - 4}, {x - 3, y}, 1, outline);
      DrawLineEx({x - 7, y + 4}, {x - 3, y}, 1, outline);
    }
    centerText(fonts.ui, labels[i], {x + width / 2, top + 15}, 14, secondary);
    centerText(fonts.mono, std::string(1, letters[i]),
               {x + width / 2, top + 39}, 25, lamp ? gold : ink);
  }
}

void drawKeyboard(const Session &session, const Layout &layout,
                  const Fonts &fonts, Vector2 cursor) {
  const auto &bounds = layout.keyboard;
  card(bounds);
  write(fonts.ui, "Lampboard", {bounds.x + 18, bounds.y + 16}, 23);
  const auto &trace = session.trace();
  if (trace) {
    const std::string lamp =
        std::string("Key ") + trace->input + "   →   Lamp " + trace->output;
    rightText(fonts.mono, lamp, bounds.x + bounds.width - 18, bounds.y + 20, 16,
              gold);
  }

  for (int i = 0; i < 26; ++i) {
    const Vector2 point = layout.keyCenters[idx(i)];
    const float radius = layout.keyRadius;
    const char letter = indexToChar(i);
    const bool input = trace && trace->input == letter;
    const bool output = trace && trace->output == letter;
    const bool over = CheckCollisionPointCircle(cursor, point, radius);
    Color fill = over ? hovered : raised;
    Color ring = over ? gold : outline;
    if (input) {
      fill = mintSurface;
      ring = mint;
    }
    if (output) {
      fill = gold;
      ring = gold;
    }
    DrawCircleV(point, radius, fill);
    DrawRing(point, radius - 1.4F, radius, 0, 360, 48, ring);
    centerText(fonts.mono, std::string(1, letter), point, 22,
               output ? backdrop : ink);
  }
}

void drawStepper(const Stepper &stepper, const std::string &value,
                 const Fonts &fonts, Vector2 cursor) {
  button(fonts.mono, stepper.decrease, "−", cursor);
  button(fonts.mono, stepper.increase, "+", cursor);
  const auto decrease = center(stepper.decrease);
  const auto increase = center(stepper.increase);
  centerText(fonts.mono, value, {(decrease.x + increase.x) / 2, decrease.y},
             18);
}

float stepperCenterX(const Stepper &stepper) {
  return (center(stepper.decrease).x + center(stepper.increase).x) / 2;
}

void drawRotors(const Session &session, const Layout &layout,
                const Fonts &fonts, Vector2 cursor) {
  const auto &first = layout.rotorRows.front();
  const float labelsY = first.bounds.y - 14;
  centerText(fonts.ui, "Rotor", {first.bounds.x + 26, labelsY}, 14, secondary);
  centerText(fonts.ui, "Live", {center(first.liveWindow).x, labelsY}, 14,
             secondary);
  centerText(fonts.ui, "Start", {stepperCenterX(first.start), labelsY}, 14,
             secondary);
  centerText(fonts.ui, "Ring", {stepperCenterX(first.ring), labelsY}, 14,
             secondary);
  const std::array<std::string, rotorCount> names{"I", "II", "III"};
  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto &row = layout.rotorRows[i];
    DrawRectangleRounded(row.bounds, 0.2F, 8, raised);
    centerText(fonts.mono, names[i], {row.bounds.x + 26, center(row.bounds).y},
               20);
    DrawRectangleRounded(row.liveWindow, 0.2F, 8, goldSurface);
    DrawRectangleRoundedLines(row.liveWindow, 0.2F, 8, 1, gold);
    centerText(fonts.mono, letterAt(session.positionAt(i)),
               center(row.liveWindow), 22, gold);
    std::string ring = std::to_string(session.ringAt(i) + 1);
    if (ring.size() == 1) {
      ring.insert(ring.begin(), '0');
    }
    drawStepper(row.start, letterAt(session.startAt(i)), fonts, cursor);
    drawStepper(row.ring, ring, fonts, cursor);
  }
}

void drawPlugboard(const Session &session, const Layout &layout,
                   const Fonts &fonts, Vector2 cursor) {
  const auto &bounds = layout.machine;
  const float x = bounds.x + 18;
  const float y = layout.plugboardTop;
  write(fonts.ui, "Plugboard", {x, y}, 21);

  const auto selected = session.selectedPlug();
  const std::string hint =
      selected ? "Connect " + letterAt(*selected) + " to a second letter"
               : "Click two letters to connect them";
  write(fonts.ui, hint, {x, y + 29}, 15, selected ? gold : secondary);
  const auto &pairs = session.plugPairs();
  if (pairs.empty()) {
    write(fonts.ui, "No leads connected", {x, y + 59}, 15, secondary);
  }
  const float cellWidth = (bounds.width - 36) / 5;
  for (std::size_t i = 0; i < pairs.size(); ++i) {
    const auto [first, second] = pairs[i];
    const float left = x + static_cast<float>(i % 5) * cellWidth;
    const float top = y + 52 + static_cast<float>(i / 5) * 20;
    write(fonts.mono, letterAt(first) + "–" + letterAt(second), {left, top}, 15,
          mint);
  }

  for (int i = 0; i < 26; ++i) {
    const Vector2 point = layout.plugCenters[idx(i)];
    const float radius = layout.plugRadius;
    const bool connected = session.isPlugConnected(i);
    const bool active = selected == i;
    const bool over = CheckCollisionPointCircle(cursor, point, radius);
    const Color fill = active ? gold : (connected ? mintSurface : raised);
    const Color edge = active || over ? gold : (connected ? mint : outline);
    DrawCircleV(point, radius, fill);
    DrawRing(point, radius - 1.2F, radius, 0, 360, 36, edge);
    centerText(fonts.mono, letterAt(i), point, 16, active ? backdrop : ink);
  }
  write(fonts.ui, "Click a connected letter to remove its lead", {x, y + 202},
        14, secondary);
}

Font loadFontAsset(const std::string &name) {
  const std::string path =
      std::string(GetApplicationDirectory()) + "assets/" + name;
  std::array<int, 99> codepoints{};
  for (int i = 0; i < 95; ++i) {
    codepoints[static_cast<std::size_t>(i)] = 32 + i;
  }
  codepoints[95] = 0x00B7; // Middle dot
  codepoints[96] = 0x2013; // En dash
  codepoints[97] = 0x2192; // Right arrow
  codepoints[98] = 0x2212; // Minus sign
  Font font = LoadFontEx(path.c_str(), 48, codepoints.data(),
                         static_cast<int>(codepoints.size()));
  if (font.texture.id == 0) {
    return GetFontDefault();
  }
  if (font.texture.id != GetFontDefault().texture.id) {
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
  }
  return font;
}

} // namespace

Fonts loadFonts() {
  return {loadFontAsset("Montserrat-Medium.ttf"),
          loadFontAsset("JetBrainsMono-Medium.ttf")};
}

void unloadFonts(const Fonts &fonts) {
  UnloadFont(fonts.ui);
  UnloadFont(fonts.mono);
}

void drawWorkbench(const Session &session, const Layout &layout,
                   const Fonts &fonts, std::string_view status, int width,
                   int height) {
  const Vector2 cursor = GetMousePosition();
  const float right = static_cast<float>(width) - margin;
  const float footerY = static_cast<float>(height) - 29;
  ClearBackground(backdrop);

  write(fonts.ui, "Enigma Workbench", {margin, 22}, 34);
  const std::string count = std::to_string(session.input().size()) + " / " +
                            std::to_string(messageLimit) + " letters";
  rightText(fonts.ui, count, right, 33, 16, secondary);

  drawMessage(session, layout, fonts, cursor);
  drawSignal(session, layout.signal, fonts);
  drawKeyboard(session, layout, fonts, cursor);
  card(layout.machine);
  write(fonts.ui, "Machine settings",
        {layout.machine.x + 18, layout.machine.y + 16}, 23);
  drawRotors(session, layout, fonts, cursor);
  drawPlugboard(session, layout, fonts, cursor);
  button(fonts.ui, layout.clearPlugboard, "Clear plugboard", cursor);
  button(fonts.ui, layout.reset, "Reset machine", cursor);

  write(fonts.ui, "Backspace  Undo     ·     Ctrl+C  Copy ciphertext",
        {margin, footerY}, 15, secondary);
  if (!status.empty()) {
    rightText(fonts.ui, std::string(status), right, footerY, 15, mint);
  }
}

} // namespace workbench
