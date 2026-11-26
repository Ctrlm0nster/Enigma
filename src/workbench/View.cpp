#include "View.hpp"

#include "Charactermap.hpp"
#include "Input.hpp"
#include "Layout.hpp"
#include "Session.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>

namespace workbench {
namespace {

constexpr Color background{17, 19, 22, 255};
constexpr Color panel{26, 29, 33, 255};
constexpr Color well{16, 18, 21, 255};
constexpr Color raised{37, 41, 47, 255};
constexpr Color hoverFill{50, 56, 65, 255};
constexpr Color border{57, 63, 72, 255};
constexpr Color keyEdge{82, 89, 99, 255};
constexpr Color shadow{11, 13, 15, 255};
constexpr Color foreground{247, 245, 238, 255};
constexpr Color muted{183, 189, 197, 255};
constexpr Color amber{255, 194, 109, 255};
constexpr Color amberHover{255, 213, 156, 255};
constexpr Color amberLow{64, 47, 30, 255};
constexpr Color amberWell{34, 28, 23, 255};
constexpr Color wireLit{201, 150, 83, 255};
constexpr Color wireIdle{110, 117, 127, 255};

constexpr std::array controlGlyphs{0x00B7, 0x2013, 0x2026, 0x2192, 0x2212};

enum class Button { normal, primary };

float right(Rectangle box) { return box.x + box.width; }
float bottom(Rectangle box) { return box.y + box.height; }

Vector2 center(Rectangle box) {
  return {box.x + box.width / 2, box.y + box.height / 2};
}

Rectangle insetBy(Rectangle box, float amount) {
  return {box.x + amount, box.y + amount, box.width - 2 * amount,
          box.height - 2 * amount};
}

void box(Rectangle bounds, Color fill, Color edge = border,
         float roundness = 0.07F) {
  DrawRectangleRounded(bounds, roundness, 8, edge);
  DrawRectangleRounded(insetBy(bounds, 1), roundness, 8, fill);
}

void rule(float left, float edge, float y) {
  DrawLineEx({left, y}, {edge, y}, 1, border);
}

void write(Font font, const std::string &value, Vector2 origin, float size,
           Color color = foreground) {
  DrawTextEx(font, value.c_str(), origin, size, 0.45F, color);
}

Vector2 measure(Font font, const std::string &value, float size) {
  return MeasureTextEx(font, value.c_str(), size, 0.45F);
}

void centerText(Font font, const std::string &value, Vector2 point, float size,
                Color color = foreground) {
  const Vector2 extent = measure(font, value, size);
  write(font, value, {point.x - extent.x / 2, point.y - extent.y / 2}, size,
        color);
}

void rightText(Font font, const std::string &value, float edge, float y,
               float size, Color color = muted) {
  write(font, value, {edge - measure(font, value, size).x, y}, size, color);
}

void button(Font font, Rectangle bounds, const std::string &caption,
            Vector2 cursor, bool enabled = true, Button kind = Button::normal) {
  const bool over = enabled && CheckCollisionPointRec(cursor, bounds);
  const bool primary = enabled && kind == Button::primary;
  const Color fill =
      primary ? (over ? amberHover : amber) : (over ? hoverFill : raised);
  box(bounds, fill, primary ? fill : border, 0.22F);
  centerText(font, caption, center(bounds), 16,
             primary ? well : (enabled ? foreground : muted));
}

std::string grouped(std::string_view message) {
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

void writeTape(Font mono, std::string_view message, Rectangle area,
               Color color) {
  constexpr float size = 26;
  constexpr std::size_t groupSize = 5;
  const float groupWidth = measure(mono, "MMMMM ", size).x;
  const auto perLine = std::max(
      std::size_t{1}, static_cast<std::size_t>(area.width / groupWidth));
  const auto groupCount = (message.size() + groupSize - 1) / groupSize;
  const bool truncated = groupCount > 2 * perLine;
  const auto start = truncated ? (groupCount - 2 * perLine + 1) * groupSize : 0;
  const auto firstLine =
      (perLine - static_cast<std::size_t>(truncated)) * groupSize;
  const auto tail = message.substr(start);
  std::string line = grouped(tail.substr(0, firstLine));
  if (truncated) {
    line.insert(0, "… ");
  }
  const bool twoLines = tail.size() > firstLine;
  const float y = area.y + (area.height - (twoLines ? 54 : size)) / 2;
  write(mono, line, {area.x, y}, size, color);
  if (twoLines) {
    write(mono, grouped(tail.substr(firstLine)), {area.x, y + 28}, size, color);
  }
}

std::string letterAt(int index) { return std::string(1, indexToChar(index)); }

void readout(const Assets &assets, Rectangle bounds, const std::string &label,
             const std::string &message, bool cipher) {
  constexpr float captionWidth = 112;
  box(bounds, cipher ? amberWell : well, cipher ? amberLow : border, 0.13F);
  const float divider = bounds.x + captionWidth;
  DrawLineEx({divider, bounds.y + 14}, {divider, bottom(bounds) - 14}, 1,
             cipher ? amberLow : border);
  write(assets.ui, label, {bounds.x + 14, bounds.y + 29}, 16,
        cipher ? amber : muted);
  const Rectangle display{divider + 13, bounds.y,
                          bounds.width - captionWidth - 26, bounds.height};
  if (!message.empty()) {
    writeTape(assets.mono, message, display, cipher ? amber : foreground);
  } else {
    write(assets.ui, cipher ? "Awaiting input" : "Type or paste a message",
          {display.x, bounds.y + 29}, 17, muted);
  }
}

void drawMessage(const Session &session, const Layout &layout,
                 const Assets &assets, Vector2 cursor) {
  const auto &bounds = layout.message;
  box(bounds, panel);
  write(assets.ui, "Message", {bounds.x + 18, bounds.y + 16}, 24);
  button(assets.ui, layout.paste, "Paste", cursor,
         session.input().size() < messageLimit);
  button(assets.ui, layout.clear, "Clear", cursor, !session.input().empty());
  button(assets.ui, layout.copy, "Copy ciphertext", cursor,
         !session.output().empty(), Button::primary);

  const Rectangle plain{bounds.x + 16, bounds.y + 57, bounds.width - 32, 74};
  const Rectangle cipher{plain.x, bottom(plain) + 8, plain.width, plain.height};
  readout(assets, plain, "Plaintext", session.input(), false);
  readout(assets, cipher, "Ciphertext", session.output(), true);
}

void drawKey(Rectangle bounds, char letter, const Assets &assets, bool pressed,
             bool lit, bool over) {
  const Rectangle base{bounds.x, bounds.y + 4, bounds.width, bounds.height};
  DrawRectangleRounded(base, 0.20F, 8, shadow);
  if (pressed && !lit) {
    bounds.y += 2;
  }
  const Color edge = lit || pressed ? amber : (over ? muted : keyEdge);
  const Color face = lit ? amber : (over ? hoverFill : raised);
  box(bounds, face, edge, 0.20F);
  if (!lit) {
    DrawLineEx({bounds.x + 10, bounds.y + 3},
               {right(bounds) - 10, bounds.y + 3}, 1, keyEdge);
  }
  centerText(assets.mono, std::string(1, letter), center(bounds), 25,
             lit ? well : foreground);
}

void drawKeyboard(const Session &session, const Layout &layout,
                  const Assets &assets, Vector2 cursor) {
  const Rectangle bounds = layout.keyboard;
  box(bounds, panel);
  write(assets.ui, "Keyboard", {bounds.x + 18, bounds.y + 17}, 24);
  const auto &trace = session.trace();
  if (trace) {
    const auto activity =
        std::string("Key ") + trace->input + "   →   Lamp " + trace->output;
    rightText(assets.mono, activity, right(bounds) - 19, bounds.y + 20, 18,
              amber);
  } else {
    rightText(assets.ui, "Type A–Z or click a key", right(bounds) - 19,
              bounds.y + 21, 16);
  }
  rule(bounds.x + 16, right(bounds) - 16, bounds.y + 57);
  const Rectangle tray{bounds.x + 16, bounds.y + 74, bounds.width - 32,
                       bounds.height - 90};
  box(tray, well);

  for (int i = 0; i < 26; ++i) {
    const Rectangle key = layout.keys[idx(i)];
    const char letter = indexToChar(i);
    drawKey(key, letter, assets, trace && trace->input == letter,
            trace && trace->output == letter,
            CheckCollisionPointRec(cursor, key));
  }
}

void arrowAt(Vector2 point, float direction, Color color) {
  const float start = point.x - 4 * direction;
  DrawLineEx({start, point.y - 3}, point, 1, color);
  DrawLineEx({start, point.y + 3}, point, 1, color);
}

void traceNode(const Assets &assets, Vector2 point, char letter,
               const char *caption, bool reflector, bool lit) {
  const Rectangle cell{point.x - 14, point.y - 14, 28, 28};
  const Color fill = lit ? amber : (reflector ? amberWell : well);
  box(cell, fill, lit || reflector ? amber : border, 0.22F);
  centerText(assets.mono, std::string(1, letter), point, 20,
             lit ? well : foreground);
  centerText(assets.ui, caption, {point.x, point.y + 25}, 13, muted);
}

void drawSignal(const Session &session, Rectangle bounds,
                const Assets &assets) {
  write(assets.ui, "Signal path", {bounds.x, bounds.y + 1}, 22);
  const auto &trace = session.trace();
  const auto count = session.rotorCount();
  const auto columnCount = count + 3;
  struct Column {
    const char *outboundLabel = nullptr;
    const char *returnLabel = nullptr;
    char outboundLetter = '-';
    char returnLetter = '-';
  };
  std::array<Column, maximumRotorCount + 3> columns{};
  columns[0] = {"Key", "Lamp", trace ? trace->input : '-',
                trace ? trace->output : '-'};
  columns[1] = {"Plug", "Plug", trace ? trace->plugboardForward : '-',
                trace ? trace->output : '-'};
  for (std::size_t i = 0; i < count; ++i) {
    // The outward signal reaches the rightmost rotor first.
    const auto rotor = count - 1 - i;
    columns[i + 2] = {rotorNames[rotor], rotorNames[rotor],
                      trace ? trace->rotorForward[i] : '-',
                      trace ? trace->rotorBackward[rotor] : '-'};
  }
  columns[columnCount - 1] = {"Refl. B", nullptr,
                              trace ? trace->reflected : '-', '-'};

  const float slot = bounds.width / static_cast<float>(columnCount);
  const float start = bounds.x + slot / 2;
  const float turn = right(bounds) - 2;
  const float top = bounds.y + 46;
  const float base = bottom(bounds) - 40;
  const Color wire = trace ? wireLit : wireIdle;
  DrawLineEx({start, top}, {turn, top}, 1, wire);
  DrawLineEx({turn, top}, {turn, base}, 1, wire);
  DrawLineEx({start, base}, {turn, base}, 1, wire);

  for (std::size_t i = 0; i < columnCount; ++i) {
    const float x = start + static_cast<float>(i) * slot;
    if (i != 0) {
      arrowAt({x - slot / 2 + 2, top}, 1, wire);
      arrowAt({x - slot / 2 - 2, base}, -1, wire);
    }
    const auto &column = columns[i];
    const bool reflector = i == columnCount - 1;
    traceNode(assets, {x, top}, column.outboundLetter, column.outboundLabel,
              reflector, false);
    if (column.returnLabel != nullptr) {
      traceNode(assets, {x, base}, column.returnLetter, column.returnLabel,
                false, i == 0 && trace.has_value());
    }
  }
  rule(bounds.x, right(bounds), bottom(bounds));
}

float stepperX(const Stepper &stepper) {
  return (center(stepper.decrease).x + center(stepper.increase).x) / 2;
}

void drawStepper(const Stepper &stepper, const std::string &value,
                 const Assets &assets, Vector2 cursor, bool canDecrease = true,
                 bool canIncrease = true) {
  button(assets.mono, stepper.decrease, "−", cursor, canDecrease);
  button(assets.mono, stepper.increase, "+", cursor, canIncrease);
  centerText(assets.mono, value,
             {stepperX(stepper), center(stepper.decrease).y}, 18);
}

void drawRotors(const Session &session, const Layout &layout,
                const Assets &assets, Vector2 cursor) {
  const auto &first = layout.rotorRows.front();
  const float x = first.bounds.x;
  const auto count = session.rotorCount();
  write(assets.ui, "Rotor settings", {x, layout.rotorsTop}, 22);
  drawStepper(layout.rotorCountControl, std::to_string(count), assets, cursor,
              count > minimumRotorCount, count < maximumRotorCount);
  const float labelsY = first.bounds.y - 10;
  centerText(assets.ui, "Rotor", {x + 19, labelsY}, 13, muted);
  centerText(assets.ui, "Live", {center(first.liveWindow).x, labelsY}, 13,
             muted);
  centerText(assets.ui, "Start", {stepperX(first.start), labelsY}, 13, muted);
  centerText(assets.ui, "Ring", {stepperX(first.ring), labelsY}, 13, muted);
  for (std::size_t i = 0; i < count; ++i) {
    const auto &row = layout.rotorRows[i];
    centerText(assets.mono, rotorNames[i], {x + 19, center(row.bounds).y}, 18);
    box(row.liveWindow, well, border, 0.22F);
    centerText(assets.mono, letterAt(session.positionAt(i)),
               center(row.liveWindow), 21, amber);
    std::string ring = std::to_string(session.ringAt(i) + 1);
    if (ring.size() == 1) {
      ring.insert(ring.begin(), '0');
    }
    drawStepper(row.start, letterAt(session.startAt(i)), assets, cursor);
    drawStepper(row.ring, ring, assets, cursor);
  }
  rule(x, right(first.bounds), layout.plugboardTop - 8);
}

void drawPlugboard(const Session &session, const Layout &layout,
                   const Assets &assets, Vector2 cursor) {
  const float x = layout.clearPlugboard.x;
  const float y = layout.plugboardTop;
  const auto &pairs = session.plugPairs();
  const auto selected = session.selectedPlug();
  write(assets.ui, "Plugboard", {x, y}, 22);
  rightText(assets.mono, std::to_string(pairs.size()) + " / 10",
            right(layout.reset), y + 5, 14);

  std::string leads;
  for (const auto &[first, second] : pairs) {
    if (!leads.empty()) {
      leads += " ";
    }
    leads += letterAt(first) + "–" + letterAt(second);
    const auto a = layout.plugCenters[idx(first)];
    const auto b = layout.plugCenters[idx(second)];
    if (CheckCollisionPointCircle(cursor, a, layout.plugRadius) ||
        CheckCollisionPointCircle(cursor, b, layout.plugRadius)) {
      DrawLineEx(a, b, 2, wireLit);
    }
  }
  if (selected) {
    write(assets.ui, "Connect " + letterAt(*selected) + " to another socket",
          {x, y + 34}, 15, amber);
  } else if (leads.empty()) {
    write(assets.ui, "Click two sockets to connect them", {x, y + 34}, 15,
          muted);
  } else {
    write(assets.mono, leads, {x, y + 36}, 12, amber);
  }

  for (int i = 0; i < 26; ++i) {
    const Vector2 point = layout.plugCenters[idx(i)];
    const float radius = layout.plugRadius;
    const bool connected = session.isPlugConnected(i);
    const bool active = selected == i;
    const bool over = CheckCollisionPointCircle(cursor, point, radius);
    const Color fill = active ? amber : (connected ? amberLow : raised);
    DrawCircleV(point, radius, active || over || connected ? amber : keyEdge);
    DrawCircleV(point, radius - 1, fill);
    centerText(assets.mono, letterAt(i), point, 16, active ? well : foreground);
  }
  button(assets.ui, layout.clearPlugboard, "Clear leads", cursor,
         !pairs.empty() || selected.has_value());
  button(assets.ui, layout.reset, "Reset machine", cursor);
}

void drawPhotoButton(const Layout &layout, const Assets &assets,
                     Vector2 cursor) {
  const bool over = CheckCollisionPointRec(cursor, layout.openPhoto);
  box(layout.openPhoto, over ? hoverFill : panel, border, 0.18F);
  if (assets.photo.id != 0) {
    const float width = static_cast<float>(assets.photo.width);
    const float height = static_cast<float>(assets.photo.height);
    const Rectangle sample{width * 0.30F, height * 0.52F, width * 0.55F,
                           height * 0.36F};
    DrawTexturePro(assets.photo, sample, layout.photo, {}, 0, WHITE);
  }
  write(assets.ui, "Machine photo",
        {right(layout.photo) + 10, layout.photo.y + 9}, 16);
}

void drawPhotoViewer(const Layout &layout, const Assets &assets,
                     Vector2 cursor) {
  const Rectangle frame = layout.photoViewer;
  DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {7, 8, 10, 205});
  box(frame, panel, keyEdge, 0.035F);
  write(assets.title, "Imperial War Museum", {frame.x + 17, frame.y + 10}, 35);
  button(assets.ui, layout.closePhoto, "Close", cursor);
  const Rectangle picture{frame.x + 16, frame.y + 56, frame.width - 32,
                          frame.height - 95};
  if (assets.photo.id != 0) {
    const float width = static_cast<float>(assets.photo.width);
    const float height = static_cast<float>(assets.photo.height);
    const float scale =
        std::min(picture.width / width, picture.height / height);
    const auto point = center(picture);
    const Rectangle target{point.x - width * scale / 2,
                           point.y - height * scale / 2, width * scale,
                           height * scale};
    DrawTexturePro(assets.photo, {0, 0, width, height}, target, {}, 0, WHITE);
  } else {
    centerText(assets.ui, "Photograph unavailable", center(picture), 17, muted);
  }
  centerText(assets.ui, "Karsten Sperling · Public domain",
             {center(frame).x, bottom(frame) - 18}, 15, muted);
}

Font loadFontAsset(const std::string &path, int pixelSize,
                   std::span<const int> extra = {}) {
  constexpr int firstAscii = 32;
  constexpr int asciiCount = 95;
  std::array<int, asciiCount + controlGlyphs.size()> codepoints{};
  for (int i = 0; i < asciiCount; ++i) {
    codepoints[static_cast<std::size_t>(i)] = firstAscii + i;
  }
  std::ranges::copy(extra, codepoints.begin() + asciiCount);
  Font font = LoadFontEx(path.c_str(), pixelSize, codepoints.data(),
                         asciiCount + static_cast<int>(extra.size()));
  if (font.texture.id == 0) {
    return GetFontDefault();
  }
  if (font.texture.id != GetFontDefault().texture.id) {
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
  }
  return font;
}

} // namespace

Assets loadAssets() {
  const std::string root = std::string(GetApplicationDirectory()) + "assets/";
  Texture2D photo =
      LoadTexture((root + "Enigma-Imperial-War-Museum.jpg").c_str());
  if (photo.id != 0) {
    GenTextureMipmaps(&photo);
    SetTextureFilter(photo, TEXTURE_FILTER_TRILINEAR);
  }
  return {loadFontAsset(root + "InstrumentSerif-Regular.ttf", 64),
          loadFontAsset(root + "Montserrat-Medium.ttf", 48, controlGlyphs),
          loadFontAsset(root + "JetBrainsMono-Medium.ttf", 44, controlGlyphs),
          photo};
}

void unloadAssets(const Assets &assets) {
  UnloadFont(assets.title);
  UnloadFont(assets.ui);
  UnloadFont(assets.mono);
  if (assets.photo.id != 0) {
    UnloadTexture(assets.photo);
  }
}

void drawWorkbench(const Session &session, const Layout &layout,
                   const Assets &assets, const InterfaceState &state) {
  const Vector2 cursor = GetMousePosition();
  const Vector2 controls = state.photoOpen ? Vector2{-1, -1} : cursor;
  const float left = layout.content.x;
  const float edge = right(layout.content);
  ClearBackground(background);

  write(assets.ui, "Enigma Workbench", {left, 18}, 37);
  std::string configuration = "Rotors ";
  for (std::size_t i = 0; i < session.rotorCount(); ++i) {
    if (i != 0) {
      configuration += ", ";
    }
    configuration += rotorNames[i];
  }
  configuration += "  ·  Reflector B";
  write(assets.ui, configuration, {left + 1, 59}, 16, muted);
  drawPhotoButton(layout, assets, controls);
  rule(left, edge, 83);

  drawMessage(session, layout, assets, controls);
  drawKeyboard(session, layout, assets, controls);
  box(layout.machine, panel);
  drawSignal(session, layout.signal, assets);
  drawRotors(session, layout, assets, controls);
  drawPlugboard(session, layout, assets, controls);

  const float footer = bottom(layout.content);
  const std::string count = std::to_string(session.input().size()) + " / " +
                            std::to_string(messageLimit) + " letters";
  rightText(assets.mono, count, edge, footer + 14, 16);
  if (const auto status = state.status.current(); !status.empty()) {
    write(assets.ui, std::string(status), {left, footer + 13}, 16, amber);
  } else {
    write(assets.ui,
          "Ctrl+V  Paste    ·    Ctrl+C  Copy    ·    Backspace  Undo",
          {left, footer + 13}, 15, muted);
  }
  if (state.photoOpen) {
    drawPhotoViewer(layout, assets, cursor);
  }
}

} // namespace workbench
