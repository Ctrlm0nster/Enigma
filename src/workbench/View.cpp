#include "View.hpp"

#include "AudioEngine.hpp"
#include "Charactermap.hpp"
#include "EnigmaSettings.hpp"
#include "ImageDecoder.hpp"
#include "Input.hpp"
#include "Layout.hpp"
#include "Session.hpp"
#include "Typography.hpp"
#include "VulkanRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace workbench {

std::string resolveAssetDirectory() {
  namespace fs = std::filesystem;
  std::string exeDir;
#if defined(__linux__)
  char buf[1024];
  const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len > 0) {
    buf[len] = '\0';
    exeDir = fs::path(buf).parent_path().string();
  }
#elif defined(_WIN32)
  char buf[MAX_PATH];
  const DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
  if (len > 0) {
    exeDir = fs::path(buf).parent_path().string();
  }
#endif

  const std::vector<std::string> candidates = {
      exeDir.empty() ? "" : exeDir + "/assets",
      exeDir.empty() ? "" : exeDir + "/../assets",
      "./assets",
      "assets",
      "../assets"};

  for (const auto &cand : candidates) {
    if (!cand.empty() && fs::exists(cand + "/InstrumentSerif-Regular.ttf")) {
      return cand;
    }
  }
  return "assets";
}

namespace {

enum class Button { normal, primary };

void rightText(const FontAtlas &font, std::string_view value, float edge,
               float centerY, float size, Color color, DrawList &drawList) {
  const Vector2 extent = font.measure(value, size);
  drawList.addText(font, value,
                   {edge - extent.x, font.centeredLineTop(centerY, size)},
                   size, color);
}

void button(const FontAtlas &font, Rectangle bounds, std::string_view caption,
            Vector2 cursor, DrawList &drawList, bool enabled = true,
            Button kind = Button::normal, float fontSize = type::control) {
  const bool over = enabled && checkCollisionPointRec(cursor, bounds);
  const bool primary = enabled && kind == Button::primary;
  const Color fill =
      primary ? (over ? amberHover : amber) : (over ? hoverFill : raised);
  drawList.addBox(bounds, fill, primary ? fill : border, 0.22F);
  drawList.addCenteredText(font, caption, center(bounds), fontSize,
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

void writeTape(const FontAtlas &mono, std::string_view message, Rectangle area,
               Color color, DrawList &drawList) {
  constexpr float size = 22;
  constexpr std::size_t groupSize = 5;
  const float groupWidth = mono.measure("MMMMM ", size).x;
  if (groupWidth <= 0.0F || area.width <= 0.0F) {
    return;
  }
  const auto perLine = std::max(
      std::size_t{1}, static_cast<std::size_t>(area.width / groupWidth));
  const auto groupCount = (message.size() + groupSize - 1) / groupSize;
  const bool truncated = groupCount > 2 * perLine;
  const auto start =
      truncated ? (groupCount - 2 * perLine + 1) * groupSize : 0;
  const auto firstLine =
      (perLine - static_cast<std::size_t>(truncated)) * groupSize;
  const auto tail = message.substr(start);
  std::string line = grouped(tail.substr(0, firstLine));
  if (truncated) {
    line.insert(0, "… ");
  }
  const bool twoLines = tail.size() > firstLine;
  constexpr float lineSpacing = 28.0F;
  const float firstCenter = center(area).y - (twoLines ? lineSpacing * 0.5F : 0);
  const float y = mono.centeredLineTop(firstCenter, size);
  drawList.setScissor(area);
  drawList.addText(mono, line, {area.x, y}, size, color);
  if (twoLines) {
    drawList.addText(mono, grouped(tail.substr(firstLine)),
                     {area.x, y + lineSpacing}, size, color);
  }
  drawList.resetScissor();
}

std::string letterAt(int index) {
  return std::string(1, indexToChar(index));
}

const char *rotorName(const Session &session, std::size_t rotor) {
  const auto model = static_cast<std::size_t>(session.rotorTypeAt(rotor));
  return model < enigma::rotorNames.size() ? enigma::rotorNames[model]
                                          : rotorNames[rotor];
}

const char *reflectorName(const Session &session) {
  const auto model = static_cast<std::size_t>(session.reflectorIndex());
  return model < enigma::reflectorNames.size() ? enigma::reflectorNames[model]
                                               : enigma::reflectorNames[1];
}

void readout(const Assets &assets, Rectangle bounds, const std::string &label,
             const std::string &message, bool cipher, DrawList &drawList) {
  constexpr float captionWidth = 112;
  drawList.addBox(bounds, cipher ? amberWell : well,
                  cipher ? amberLow : border, 0.13F);
  const float divider = bounds.x + captionWidth;
  drawList.addLine({divider, bounds.y + 14}, {divider, bottom(bounds) - 14}, 1,
                   cipher ? amberLow : border);

  const float labelY = assets.ui.centeredLineTop(center(bounds).y, type::control);
  drawList.addText(assets.ui, label, {bounds.x + 16, labelY}, type::control,
                   cipher ? amber : muted);

  const Rectangle display{divider + 16, bounds.y,
                          bounds.width - captionWidth - 32, bounds.height};
  if (!message.empty()) {
    writeTape(assets.mono, message, display, cipher ? amber : foreground,
              drawList);
  } else {
    const float phY = assets.ui.centeredLineTop(center(bounds).y, type::body);
    drawList.addText(assets.ui,
                     cipher ? "Awaiting input" : "Type or paste a message",
                     {display.x, phY}, type::body, muted);
  }
}

void drawMessage(const Session &session, const Layout &layout,
                 const Assets &assets, Vector2 cursor, DrawList &drawList) {
  const auto &bounds = layout.message;
  drawList.addBox(bounds, panel);
  const float headingY = assets.ui.centeredLineTop(center(layout.paste).y,
                                                    type::section);
  drawList.addText(assets.ui, "Message", {bounds.x + 18, headingY},
                   type::section);
  button(assets.ui, layout.createChallenge, "Create challenge", cursor,
         drawList, session.input().size() >= 2);
  button(assets.ui, layout.paste, "Paste", cursor, drawList,
         session.input().size() < messageLimit);
  button(assets.ui, layout.clear, "Clear", cursor, drawList,
         !session.input().empty());
  button(assets.ui, layout.copy, "Copy ciphertext", cursor, drawList,
         !session.output().empty(), Button::primary);

  const Rectangle plain{bounds.x + 16, bounds.y + 48, bounds.width - 32, 55};
  const Rectangle cipher{plain.x, bottom(plain) + 7, plain.width, plain.height};
  readout(assets, plain, "Plaintext", session.input(), false, drawList);
  readout(assets, cipher, "Ciphertext", session.output(), true, drawList);
}

void drawKey(Rectangle bounds, char letter, const Assets &assets, bool pressed,
             bool over, DrawList &drawList) {
  Vector2 keyCenter = center(bounds);
  const float radius = bounds.width * 0.44F;

  if (pressed) {
    keyCenter.y += 3.0F;
    drawList.addCircle({keyCenter.x, keyCenter.y + 2.0F}, radius, Color{5, 6, 8, 200});
    drawList.addCircle(keyCenter, radius, amberLow);
    drawList.addCircle(keyCenter, radius - 1.5F, Color{75, 78, 85, 255});
    drawList.addCircle(keyCenter, radius - 3.0F, Color{22, 24, 28, 255});
    drawList.addCenteredGlyph(assets.mono, letter, keyCenter,
                             radius * 1.30F, amber);
  } else {
    drawList.addCircle({keyCenter.x, keyCenter.y + 5.0F}, radius + 0.5F, Color{6, 7, 9, 160});
    drawList.addCircle(keyCenter, radius, over ? muted : keyEdge);
    drawList.addCircle(keyCenter, radius - 1.5F, raised);
    drawList.addCircle(keyCenter, radius - 3.0F, over ? Color{38, 41, 47, 255} : Color{25, 27, 31, 255});
    drawList.addCircle(keyCenter, radius - 5.0F, over ? hoverFill : raised);
    drawList.addCenteredGlyph(assets.mono, letter, keyCenter,
                             radius * 1.30F, over ? amber : foreground);
  }
}

void drawLampboard(const Session &session, const Layout &layout,
                   const Assets &assets, DrawList &drawList) {
  const Rectangle bounds = layout.lampboard;
  drawList.addBox(bounds, panel);
  const float headingCenter = bounds.y + 20;
  const float headingY = assets.ui.centeredLineTop(headingCenter, type::section);
  drawList.addText(assets.ui, "Lampboard", {bounds.x + 18, headingY},
                   type::section);
  const auto &trace = session.trace();
  if (trace) {
    const auto activity = std::string("Lit: ") + trace->output;
    rightText(assets.mono, activity, right(bounds) - 18, headingCenter,
              type::body, amber, drawList);
  }
  drawList.addLine({bounds.x + 16, bounds.y + 36},
                   {right(bounds) - 16, bounds.y + 36}, 1, border);
  const Rectangle tray{bounds.x + 16, bounds.y + 38, bounds.width - 32,
                       bounds.height - 46};
  drawList.addBox(tray, well);

  const float radius = layout.lampRadius;
  if (trace) {
    const auto lamp = layout.lampCenters[idx(charToIndex(trace->output))];
    drawList.setScissor(insetBy(tray, 1));
    drawList.addBloomDisc(lamp, radius * 0.7F, radius * 2.5F,
                          Color{255, 170, 51, 135}, Color{255, 120, 20, 0});
    drawList.addBloomDisc(lamp, radius * 0.3F, radius * 1.5F,
                          Color{255, 215, 110, 195}, Color{255, 160, 40, 0});
    drawList.resetScissor();
  }

  for (int i = 0; i < 26; ++i) {
    const Vector2 lampCenter = layout.lampCenters[idx(i)];
    const char letter = indexToChar(i);
    const bool lit = trace && trace->output == letter;
    if (lit) {
      drawList.addCircle(lampCenter, radius + 2.0F, Color{230, 170, 60, 255});
      drawList.addCircle(lampCenter, radius, Color{255, 235, 150, 255});
      drawList.addCenteredGlyph(assets.mono, letter, lampCenter,
                               radius * 1.35F, Color{30, 20, 10, 255});
    } else {
      drawList.addCircle(lampCenter, radius + 2.0F, Color{55, 48, 38, 255});
      drawList.addCircle(lampCenter, radius + 0.5F, Color{35, 30, 24, 255});
      drawList.addCircle(lampCenter, radius - 0.5F, Color{18, 16, 14, 255});
      drawList.addCircle(lampCenter, radius - 3.0F, Color{24, 22, 19, 255});
      drawList.addCenteredGlyph(assets.mono, letter, lampCenter,
                               radius * 1.25F, Color{161, 149, 133, 220});
    }
  }
}

void drawKeyboard(const Session &session, const Layout &layout,
                  const Assets &assets, const InterfaceState &state,
                  Vector2 cursor, DrawList &drawList) {
  const Rectangle bounds = layout.keyboard;
  drawList.addBox(bounds, panel);
  const float headingCenter = bounds.y + 20;
  const float headingY = assets.ui.centeredLineTop(headingCenter, type::section);
  drawList.addText(assets.ui, "Keyboard", {bounds.x + 18, headingY},
                   type::section);
  const auto &trace = session.trace();
  if (state.pressedKey.has_value()) {
    const auto activity = std::string("Pressed: ") + *state.pressedKey;
    rightText(assets.mono, activity, right(bounds) - 18, headingCenter,
              type::body, amber, drawList);
  } else if (trace) {
    const auto activity = std::string("Last key: ") + trace->input;
    rightText(assets.mono, activity, right(bounds) - 18, headingCenter,
              type::body, muted, drawList);
  } else {
    rightText(assets.ui, "Type A–Z or click a key", right(bounds) - 18,
              headingCenter, type::caption, muted, drawList);
  }
  drawList.addLine({bounds.x + 16, bounds.y + 36},
                   {right(bounds) - 16, bounds.y + 36}, 1, border);
  const Rectangle tray{bounds.x + 16, bounds.y + 38, bounds.width - 32,
                       bounds.height - 46};
  drawList.addBox(tray, well);

  for (int i = 0; i < 26; ++i) {
    const Rectangle key = layout.keys[idx(i)];
    const char letter = indexToChar(i);
    const bool pressed = state.pressedKey == letter;
    const bool over = checkCollisionPointRec(cursor, key);
    drawKey(key, letter, assets, pressed, over, drawList);
  }
}

void arrowAt(Vector2 point, float direction, Color color, DrawList &drawList) {
  const float start = point.x - 4 * direction;
  drawList.addLine({start, point.y - 3}, point, 1, color);
  drawList.addLine({start, point.y + 3}, point, 1, color);
}

void traceNode(const Assets &assets, Vector2 point, char letter,
               const char *caption, bool reflector, bool lit,
               DrawList &drawList) {
  const Rectangle cell{point.x - 13, point.y - 13, 26, 26};
  const Color fill = lit ? amber : (reflector ? amberWell : well);
  drawList.addBox(cell, fill, lit || reflector ? amber : border, 0.22F);
  drawList.addCenteredText(assets.mono, std::string(1, letter), point, 18,
                           lit ? well : foreground);
  drawList.addCenteredText(assets.ui, caption, {point.x, point.y + 21},
                           type::caption, muted);
}

void drawSignal(const Session &session, Rectangle bounds, const Assets &assets,
                DrawList &drawList) {
  drawList.addBox(bounds, panel);
  const auto headingY = assets.ui.centeredLineTop(center(bounds).y, type::section);
  drawList.addText(assets.ui, "Signal path", {bounds.x + 16, headingY},
                   type::section);
  const float divider = bounds.x + 145;
  drawList.addLine({divider, bounds.y + 14}, {divider, bottom(bounds) - 14}, 1,
                   border);
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
    const auto rotor = count - 1 - i;
    const char *rName = rotorName(session, rotor);
    columns[i + 2] = {rName, rName,
                      trace ? trace->rotorForward[i] : '-',
                      trace ? trace->rotorBackward[rotor] : '-'};
  }
  columns[columnCount - 1] = {reflectorName(session), nullptr,
                              trace ? trace->reflected : '-', '-'};

  const float turn = right(bounds) - 22;
  const float slot = (turn - divider - 10) / static_cast<float>(columnCount);
  const float start = divider + 10 + slot * 0.5F;
  const float top = bounds.y + 22;
  const float base = bounds.y + 64;
  const Color wire = trace ? wireLit : wireIdle;

  drawList.addLine({start, top}, {turn, top}, 1, wire);
  drawList.addLine({turn, top}, {turn, base}, 1, wire);
  drawList.addLine({start, base}, {turn, base}, 1, wire);

  for (std::size_t i = 0; i < columnCount; ++i) {
    const float x = start + static_cast<float>(i) * slot;
    if (i != 0) {
      arrowAt({x - slot * 0.5F + 2, top}, 1, wire, drawList);
      arrowAt({x - slot * 0.5F - 2, base}, -1, wire, drawList);
    }
    const auto &column = columns[i];
    const bool reflector = i == columnCount - 1;
    traceNode(assets, {x, top}, column.outboundLetter, column.outboundLabel,
              reflector, false, drawList);
    if (column.returnLabel != nullptr) {
      traceNode(assets, {x, base}, column.returnLetter, column.returnLabel,
                false, i == 0 && trace.has_value(), drawList);
    }
  }
}

float stepperX(const Stepper &stepper) {
  return (center(stepper.decrease).x + center(stepper.increase).x) * 0.5F;
}

void drawStepper(const Stepper &stepper, const std::string &value,
                 const Assets &assets, Vector2 cursor, DrawList &drawList,
                 bool canDecrease = true, bool canIncrease = true) {
  button(assets.mono, stepper.decrease, "−", cursor, drawList, canDecrease,
         Button::normal, 17.0F);
  button(assets.mono, stepper.increase, "+", cursor, drawList, canIncrease,
         Button::normal, 17.0F);
  drawList.addCenteredText(assets.mono, value,
                           {stepperX(stepper), center(stepper.decrease).y}, 18);
}

void drawRotors(const Session &session, const Layout &layout,
                const Assets &assets, Vector2 cursor, DrawList &drawList) {
  drawList.addBox(layout.rotors, panel);
  const auto &first = layout.rotorRows.front();
  const float x = first.bounds.x;
  const auto count = session.rotorCount();
  const float headingY = assets.ui.centeredLineTop(
      center(layout.rotorCountControl.decrease).y, type::section);
  drawList.addText(assets.ui, "Rotors", {x, headingY}, type::section);
  button(assets.ui, layout.reflectorButton, reflectorName(session), cursor,
         drawList);
  drawStepper(layout.rotorCountControl, std::to_string(count), assets, cursor,
              drawList, count > minimumRotorCount, count < maximumRotorCount);
  const float labelsY = first.bounds.y - 12;
  drawList.addCenteredText(assets.ui, "Model", {center(first.modelButton).x, labelsY},
                           type::caption, muted);
  drawList.addCenteredText(assets.ui, "Live", {center(first.liveWindow).x, labelsY},
                           type::caption, muted);
  drawList.addCenteredText(assets.ui, "Start", {stepperX(first.start), labelsY},
                           type::caption, muted);
  drawList.addCenteredText(assets.ui, "Ring", {stepperX(first.ring), labelsY},
                           type::caption, muted);

  for (std::size_t i = 0; i < count; ++i) {
    const auto &row = layout.rotorRows[i];
    button(assets.mono, row.modelButton, rotorName(session, i), cursor, drawList);
    drawList.addBox(row.liveWindow, well, border, 0.22F);

    for (int offset = 4;
         static_cast<float>(offset) < row.liveWindow.height - 3.0F;
         offset += 4) {
      const float notchY = row.liveWindow.y + static_cast<float>(offset);
      drawList.addLine({row.liveWindow.x + 1.0F, notchY},
                       {row.liveWindow.x + 3.0F, notchY}, 1, Color{75, 80, 90, 200});
      drawList.addLine({right(row.liveWindow) - 3.0F, notchY},
                       {right(row.liveWindow) - 1.0F, notchY}, 1, Color{75, 80, 90, 200});
    }

    drawList.addCenteredText(assets.mono, letterAt(session.positionAt(i)),
                             center(row.liveWindow), 21, amber);
    std::string ring = std::to_string(session.ringAt(i) + 1);
    if (ring.size() == 1) {
      ring.insert(ring.begin(), '0');
    }
    drawStepper(row.start, letterAt(session.startAt(i)), assets, cursor,
                drawList);
    drawStepper(row.ring, ring, assets, cursor, drawList);
  }
}

void drawPlugboard(const Session &session, const Layout &layout,
                   const Assets &assets, Vector2 cursor, DrawList &drawList) {
  const float x = layout.clearPlugboard.x;
  const auto bounds = layout.plugboard;
  drawList.addBox(bounds, panel);
  const float headingCenter = bounds.y + 24;
  const float y = assets.ui.centeredLineTop(headingCenter, type::section);
  const auto &pairs = session.plugPairs();
  const auto selected = session.selectedPlug();
  drawList.addText(assets.ui, "Plugboard", {x, y}, type::section);
  rightText(assets.mono, std::to_string(pairs.size()) + " / 10",
            right(layout.reset), headingCenter, type::control, muted, drawList);

  drawList.setScissor({bounds.x + 8, bounds.y + 52, bounds.width - 16,
                        layout.clearPlugboard.y - bounds.y - 58});
  std::string leads;
  for (const auto &[first, second] : pairs) {
    if (!leads.empty()) {
      leads += ' ';
    }
    leads += letterAt(first) + "–" + letterAt(second);
    const auto a = layout.plugCenters[idx(first)];
    const auto b = layout.plugCenters[idx(second)];
    const bool hover = checkCollisionPointCircle(cursor, a, layout.plugRadius + 2.0F) ||
                       checkCollisionPointCircle(cursor, b, layout.plugRadius + 2.0F);

    const float dist = std::hypot(b.x - a.x, b.y - a.y);
    const float spaceBelow = layout.clearPlugboard.y - std::max(a.y, b.y);
    const float sag = std::min(18.0F + dist * 0.28F, spaceBelow * 0.68F);
    const Vector2 c1{a.x, a.y + sag};
    const Vector2 c2{b.x, b.y + sag};

    const Color cordColor = hover ? amberHover : Color{139, 113, 78, 125};
    drawList.addCubicBezier(a, c1, c2, b, hover ? 3.0F : 1.5F, cordColor);
  }

  if (selected && checkCollisionPointRec(cursor, bounds)) {
    const auto a = layout.plugCenters[idx(*selected)];
    const auto b = cursor;
    const float dist = std::hypot(b.x - a.x, b.y - a.y);
    const float sag = 15.0F + dist * 0.22F;
    const Vector2 c1{a.x, a.y + sag};
    const Vector2 c2{b.x, b.y + sag};
    drawList.addCubicBezier(a, c1, c2, b, 2.5F, amber);
    drawList.addCircle(a, layout.plugRadius + 2.0F, amber);
  }
  drawList.resetScissor();

  if (selected) {
    drawList.addText(assets.ui, "Connect " + letterAt(*selected) + " to another socket",
                     {x, bounds.y + 39}, type::control, amber);
  } else if (leads.empty()) {
    drawList.addText(assets.ui, "Click two sockets to connect them", {x, bounds.y + 39},
                     type::control, muted);
  } else {
    drawList.addText(assets.mono, leads, {x, bounds.y + 39}, type::caption, amber);
  }

  for (int i = 0; i < 26; ++i) {
    const Vector2 point = layout.plugCenters[idx(i)];
    const float radius = layout.plugRadius;
    const bool connected = session.isPlugConnected(i);
    const bool active = selected == i;
    const bool over = checkCollisionPointCircle(cursor, point, radius);
    const Color collar = active || over ? amber : (connected ? wireLit : keyEdge);
    const Color fill = active ? amber : (connected ? amberLow : raised);
    drawList.addCircle(point, radius + 1.0F, collar);
    drawList.addCircle(point, radius, fill);
    drawList.addCenteredText(assets.mono, letterAt(i), point, type::body,
                             active ? well : foreground);
  }
  button(assets.ui, layout.clearPlugboard, "Clear leads", cursor, drawList,
         !pairs.empty() || selected.has_value());
  button(assets.ui, layout.reset, "Reset machine", cursor, drawList);
}

void drawPresetsMenu(const Layout &layout, const Assets &assets, Vector2 cursor,
                     DrawList &drawList) {
  const auto presets = enigma::historicalPresets();
  const Rectangle menuBounds = layout.presetsMenu;

  drawList.addRectangleRounded(
      {menuBounds.x, menuBounds.y + 4, menuBounds.width, menuBounds.height},
      0.08F, 8, Color{5, 6, 8, 200});

  drawList.addBox(menuBounds, panel, keyEdge);

  const float titleY = assets.ui.centeredLineTop(menuBounds.y + 17, type::control);
  drawList.addText(assets.ui, "Historical presets", {menuBounds.x + 18, titleY},
                   type::control);
  drawList.addLine({menuBounds.x + 16, menuBounds.y + 32},
                   {right(menuBounds) - 16, menuBounds.y + 32}, 1, border);

  for (std::size_t i = 0; i < presets.size(); ++i) {
    const Rectangle itemBounds = layout.presetRow(i);
    const bool over = checkCollisionPointRec(cursor, itemBounds);
    if (over) {
      drawList.addBox(itemBounds, hoverFill, border, 0.12F);
    }

    const auto &p = presets[i];
    const float textX = itemBounds.x + 10;
    const float nameCenter = itemBounds.y + 14;
    const float nameY = assets.ui.centeredLineTop(nameCenter, type::control);
    drawList.addText(assets.ui, p.name, {textX, nameY}, type::control,
                     over ? amber : foreground);
    rightText(assets.mono, p.date, right(itemBounds) - 10, nameCenter,
              type::caption, muted, drawList);

    const float textWidth = itemBounds.width - 20;
    const std::string_view description = p.description;
    std::size_t lineEnd = description.size();
    while (lineEnd > 0 &&
           assets.ui.measure(description.substr(0, lineEnd), type::caption).x >
               textWidth) {
      const auto space = description.rfind(' ', lineEnd - 1);
      lineEnd = space == std::string_view::npos ? lineEnd - 1 : space;
    }
    const float descY = assets.ui.centeredLineTop(itemBounds.y + 32, type::caption);
    const Color descColor = over ? foreground : muted;
    drawList.addText(assets.ui, description.substr(0, lineEnd), {textX, descY},
                     type::caption, descColor);
    if (lineEnd < description.size()) {
      const auto next = description[lineEnd] == ' ' ? lineEnd + 1 : lineEnd;
      drawList.addText(assets.ui, description.substr(next), {textX, descY + 16},
                       type::caption, descColor);
    }
  }
}

void drawPhotoButton(const Layout &layout, const Assets &assets, Vector2 cursor,
                     DrawList &drawList) {
  const bool over = checkCollisionPointRec(cursor, layout.openPhoto);
  drawList.addBox(layout.openPhoto, over ? hoverFill : panel, border, 0.18F);
  float labelCenterX = center(layout.openPhoto).x;
  if (assets.photoAvailable && assets.photoTextureId != 0) {
    const float width = static_cast<float>(assets.photoWidth);
    const float height = static_cast<float>(assets.photoHeight);
    const Rectangle sample{width * 0.30F, height * 0.52F, width * 0.55F,
                           height * 0.36F};
    drawList.addTexturedQuad(assets.photoTextureId, sample, layout.photo,
                             white, {width, height});
    labelCenterX = (right(layout.photo) + 4 + right(layout.openPhoto)) * 0.5F;
  }
  drawList.addCenteredText(assets.ui, "Machine photo",
                           {labelCenterX, center(layout.openPhoto).y},
                           type::control);
}

void drawPhotoViewer(const Layout &layout, const Assets &assets, Vector2 cursor,
                     DrawList &drawList, int screenWidth, int screenHeight) {
  const Rectangle frame = layout.photoViewer;
  drawList.addRectangle({0, 0, static_cast<float>(screenWidth),
                         static_cast<float>(screenHeight)},
                        Color{7, 8, 10, 205});
  drawList.addBox(frame, panel, keyEdge, 0.035F);
  drawList.addText(assets.title, "Imperial War Museum",
                   {frame.x + 17, frame.y + 10}, 35);
  button(assets.ui, layout.closePhoto, "Close", cursor, drawList);

  const Rectangle picture{frame.x + 16, frame.y + 56, frame.width - 32,
                          frame.height - 95};
  if (assets.photoAvailable && assets.photoTextureId != 0) {
    const float width = static_cast<float>(assets.photoWidth);
    const float height = static_cast<float>(assets.photoHeight);
    const float scale =
        std::min(picture.width / width, picture.height / height);
    const auto point = center(picture);
    const Rectangle target{point.x - width * scale * 0.5F,
                           point.y - height * scale * 0.5F, width * scale,
                           height * scale};
    drawList.addTexturedQuad(assets.photoTextureId, {0, 0, width, height},
                             target, white, {width, height});
  } else {
    drawList.addCenteredText(assets.ui, "Photograph unavailable", center(picture),
                             type::body, muted);
  }
  drawList.addCenteredText(assets.ui, "Karsten Sperling · Public domain",
                           {center(frame).x, bottom(frame) - 18},
                           type::control, muted);
}

} // namespace

bool loadFontWithFallback(FontAtlas &atlas, const std::string &root,
                          const std::string &primaryFace, int size,
                          std::span<const uint32_t> extraCodepoints) {
  const std::vector<std::string> searchFaces = {
      primaryFace, "Montserrat-Medium.ttf", "JetBrainsMono-Medium.ttf",
      "InstrumentSerif-Regular.ttf"};

  for (const auto &face : searchFaces) {
    if (atlas.load(root + face, size, extraCodepoints)) {
      if (face != primaryFace) {
        std::clog << "[Assets] Font '" << primaryFace
                  << "' unavailable, using fallback '" << face << "'"
                  << '\n';
      }
      return true;
    }
  }
  return false;
}

namespace {

int scaledFontSize(int baseSize, float dpiScale) {
  const float clamped = std::clamp(dpiScale, 1.0F, 4.0F);
  return std::max(1, static_cast<int>(std::lround(static_cast<float>(baseSize) *
                                                  clamped)));
}

void uploadAtlas(VulkanRenderer &renderer, FontAtlas &atlas) {
  if (!atlas.bitmap().empty()) {
    atlas.setTextureId(renderer.createTexture(
        atlas.bitmap().data(), atlas.atlasWidth(), atlas.atlasHeight(), false,
        true));
  }
}

void destroyFontTextures(VulkanRenderer &renderer, const Assets &assets) {
  if (assets.title.textureId() != 0) {
    renderer.destroyTexture(assets.title.textureId());
  }
  if (assets.ui.textureId() != 0) {
    renderer.destroyTexture(assets.ui.textureId());
  }
  if (assets.mono.textureId() != 0) {
    renderer.destroyTexture(assets.mono.textureId());
  }
}

} // namespace

Assets loadAssets(VulkanRenderer &renderer, const std::string &assetDir,
                  float dpiScale) {
  const std::string root =
      (assetDir.empty() ? resolveAssetDirectory() : assetDir) + "/";

  Assets assets{};
  const bool titleOk = loadFontWithFallback(
      assets.title, root, "InstrumentSerif-Regular.ttf",
      scaledFontSize(64, dpiScale));
  const bool uiOk = loadFontWithFallback(
      assets.ui, root, "Montserrat-Medium.ttf",
      scaledFontSize(48, dpiScale), controlGlyphs);
  const bool monoOk = loadFontWithFallback(
      assets.mono, root, "JetBrainsMono-Medium.ttf",
      scaledFontSize(44, dpiScale), controlGlyphs);

  if (!titleOk && !uiOk && !monoOk) {
    std::cerr << "Fatal: No font faces could be loaded from " << root
              << '\n';
    assets.fontsLoaded = false;
    return assets;
  }
  assets.fontsLoaded = true;

  if (titleOk) {
    uploadAtlas(renderer, assets.title);
  }
  if (uiOk) {
    uploadAtlas(renderer, assets.ui);
  }
  if (monoOk) {
    uploadAtlas(renderer, assets.mono);
  }

  const auto photo = decodeJpeg(root + "Enigma-Imperial-War-Museum.jpg");
  if (photo.valid) {
    assets.photoWidth = photo.width;
    assets.photoHeight = photo.height;
    assets.photoTextureId = renderer.createTexture(
        photo.rgba.data(), photo.width, photo.height, true, false);
    assets.photoAvailable = (assets.photoTextureId != 0);
  } else {
    assets.photoAvailable = false;
  }

  return assets;
}

bool reloadFontAtlases(VulkanRenderer &renderer, Assets &assets, float dpiScale,
                       const std::string &assetDir) {
  const std::string root =
      (assetDir.empty() ? resolveAssetDirectory() : assetDir) + "/";

  FontAtlas nextTitle;
  FontAtlas nextUi;
  FontAtlas nextMono;
  const bool titleOk = loadFontWithFallback(
      nextTitle, root, "InstrumentSerif-Regular.ttf",
      scaledFontSize(64, dpiScale));
  const bool uiOk = loadFontWithFallback(
      nextUi, root, "Montserrat-Medium.ttf",
      scaledFontSize(48, dpiScale), controlGlyphs);
  const bool monoOk = loadFontWithFallback(
      nextMono, root, "JetBrainsMono-Medium.ttf",
      scaledFontSize(44, dpiScale), controlGlyphs);

  if (!titleOk && !uiOk && !monoOk) {
    return false;
  }

  destroyFontTextures(renderer, assets);

  if (titleOk) {
    uploadAtlas(renderer, nextTitle);
    assets.title = std::move(nextTitle);
  }
  if (uiOk) {
    uploadAtlas(renderer, nextUi);
    assets.ui = std::move(nextUi);
  }
  if (monoOk) {
    uploadAtlas(renderer, nextMono);
    assets.mono = std::move(nextMono);
  }
  assets.fontsLoaded = true;
  return true;
}

void unloadAssets(VulkanRenderer &renderer, const Assets &assets) {
  destroyFontTextures(renderer, assets);
  if (assets.photoTextureId != 0) {
    renderer.destroyTexture(assets.photoTextureId);
  }
}

void drawWorkbench(const Session &session, const Layout &layout,
                   const Assets &assets, const InterfaceState &state,
                   Vector2 cursor, DrawList &drawList, int screenWidth,
                   int screenHeight, const AudioEngine *audio) {
  const Vector2 controls = (state.photoOpen || state.presetsOpen) ? Vector2{-1.0F, -1.0F} : cursor;
  const float left = layout.content.x;
  const float edge = right(layout.content);

  const float titleY = assets.title.centeredLineTop(center(layout.openPhoto).y - 3,
                                                    type::title);
  drawList.addText(assets.title, "Enigma Workbench", {left, titleY}, type::title);

  button(assets.ui, layout.presetsButton, "Presets", state.photoOpen ? Vector2{-1.0F, -1.0F} : cursor,
         drawList, true, state.presetsOpen ? Button::primary : Button::normal);

  const bool soundMuted = audio ? audio->isMuted() : false;
  button(assets.ui, layout.soundToggle, soundMuted ? "Sound off" : "Sound on",
         controls, drawList);

  drawPhotoButton(layout, assets, state.photoOpen ? Vector2{-1.0F, -1.0F} : cursor, drawList);
  drawList.addLine({left, 76}, {edge, 76}, 1, border);

  drawMessage(session, layout, assets, controls, drawList);
  drawLampboard(session, layout, assets, drawList);
  drawKeyboard(session, layout, assets, state, controls, drawList);
  drawRotors(session, layout, assets, controls, drawList);
  drawPlugboard(session, layout, assets, controls, drawList);
  drawSignal(session, layout.signal, assets, drawList);

  const float footer = bottom(layout.content);
  const std::string count = std::to_string(session.input().size()) + " / " +
                            std::to_string(messageLimit) + " letters";
  rightText(assets.mono, count, edge, footer + 23, type::control, muted,
            drawList);
  const float footerTop = assets.ui.centeredLineTop(footer + 23, type::control);

  if (const auto status = state.status.current(); !status.empty()) {
    drawList.addText(assets.ui, status, {left, footerTop}, type::control,
                     amber);
  } else {
    drawList.addText(
        assets.ui,
        "Ctrl+V  Paste    ·    Ctrl+C  Copy    ·    Backspace  Undo",
        {left, footerTop}, type::control, muted);
  }

  if (state.presetsOpen) {
    drawPresetsMenu(layout, assets, cursor, drawList);
  }

  if (state.photoOpen) {
    drawPhotoViewer(layout, assets, cursor, drawList, screenWidth,
                    screenHeight);
  }
}

} // namespace workbench
