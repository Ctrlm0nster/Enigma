#include "View.hpp"

#include "Charactermap.hpp"
#include "ImageDecoder.hpp"
#include "Input.hpp"
#include "Layout.hpp"
#include "Session.hpp"
#include "Typography.hpp"
#include "VulkanRenderer.hpp"

#include <algorithm>
#include <array>
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
  button(assets.ui, layout.paste, "Paste", cursor, drawList,
         session.input().size() < messageLimit);
  button(assets.ui, layout.clear, "Clear", cursor, drawList,
         !session.input().empty());
  button(assets.ui, layout.copy, "Copy ciphertext", cursor, drawList,
         !session.output().empty(), Button::primary);

  const Rectangle plain{bounds.x + 16, bounds.y + 57, bounds.width - 32, 74};
  const Rectangle cipher{plain.x, bottom(plain) + 8, plain.width, plain.height};
  readout(assets, plain, "Plaintext", session.input(), false, drawList);
  readout(assets, cipher, "Ciphertext", session.output(), true, drawList);
}

void drawKey(Rectangle bounds, char letter, const Assets &assets, bool pressed,
             bool lit, bool over, DrawList &drawList) {
  const Rectangle base{bounds.x, bounds.y + 4, bounds.width, bounds.height};
  drawList.addRectangleRounded(base, 0.20F, 8, shadow);
  if (pressed && !lit) {
    bounds.y += 2;
  }
  const Color edge = lit || pressed ? amber : (over ? muted : keyEdge);
  const Color face = lit ? amber : (over ? hoverFill : raised);
  drawList.addBox(bounds, face, edge, 0.20F);
  if (!lit) {
    drawList.addLine({bounds.x + 10, bounds.y + 3},
                     {right(bounds) - 10, bounds.y + 3}, 1, keyEdge);
  }
  drawList.addCenteredText(assets.mono, std::string(1, letter), center(bounds),
                           24, lit ? well : foreground);
}

void drawKeyboard(const Session &session, const Layout &layout,
                  const Assets &assets, Vector2 cursor, DrawList &drawList) {
  const Rectangle bounds = layout.keyboard;
  drawList.addBox(bounds, panel);
  const float headingCenter = bounds.y + 30;
  const float headingY = assets.ui.centeredLineTop(headingCenter, type::section);
  drawList.addText(assets.ui, "Keyboard", {bounds.x + 18, headingY},
                   type::section);
  const auto &trace = session.trace();
  if (trace) {
    const auto activity =
        std::string("Key ") + trace->input + "   →   Lamp " + trace->output;
    rightText(assets.mono, activity, right(bounds) - 18, headingCenter,
              type::body, amber, drawList);
  } else {
    rightText(assets.ui, "Type A–Z or click a key", right(bounds) - 18,
              headingCenter, type::control, muted, drawList);
  }
  drawList.addLine({bounds.x + 16, bounds.y + 57},
                   {right(bounds) - 16, bounds.y + 57}, 1, border);
  const Rectangle tray{bounds.x + 16, bounds.y + 74, bounds.width - 32,
                       bounds.height - 90};
  drawList.addBox(tray, well);

  for (int i = 0; i < 26; ++i) {
    const Rectangle key = layout.keys[idx(i)];
    const char letter = indexToChar(i);
    drawKey(key, letter, assets, trace && trace->input == letter,
            trace && trace->output == letter,
            checkCollisionPointRec(cursor, key), drawList);
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
  const Rectangle cell{point.x - 14, point.y - 14, 28, 28};
  const Color fill = lit ? amber : (reflector ? amberWell : well);
  drawList.addBox(cell, fill, lit || reflector ? amber : border, 0.22F);
  drawList.addCenteredText(assets.mono, std::string(1, letter), point, 18,
                           lit ? well : foreground);
  drawList.addCenteredText(assets.ui, caption, {point.x, point.y + 24},
                           type::caption, muted);
}

void drawSignal(const Session &session, Rectangle bounds, const Assets &assets,
                DrawList &drawList) {
  drawList.addText(assets.ui, "Signal path", {bounds.x, bounds.y + 1},
                   type::section);
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
    columns[i + 2] = {rotorNames[rotor], rotorNames[rotor],
                      trace ? trace->rotorForward[i] : '-',
                      trace ? trace->rotorBackward[rotor] : '-'};
  }
  columns[columnCount - 1] = {"Refl. B", nullptr,
                              trace ? trace->reflected : '-', '-'};

  const float slot = bounds.width / static_cast<float>(columnCount);
  const float start = bounds.x + slot * 0.5F;
  const float turn = right(bounds) - 2;
  const float top = bounds.y + 46;
  const float base = bottom(bounds) - 40;
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
  drawList.addLine({bounds.x, bottom(bounds)},
                   {right(bounds), bottom(bounds)}, 1, border);
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
  const auto &first = layout.rotorRows.front();
  const float x = first.bounds.x;
  const auto count = session.rotorCount();
  const float headingY = assets.ui.centeredLineTop(
      center(layout.rotorCountControl.decrease).y, type::section);
  drawList.addText(assets.ui, "Rotor settings", {x, headingY}, type::section);
  drawStepper(layout.rotorCountControl, std::to_string(count), assets, cursor,
              drawList, count > minimumRotorCount, count < maximumRotorCount);
  const float labelsY = first.bounds.y - 12;
  drawList.addCenteredText(assets.ui, "Rotor", {x + 19, labelsY}, type::caption,
                           muted);
  drawList.addCenteredText(assets.ui, "Live", {center(first.liveWindow).x, labelsY},
                           type::caption, muted);
  drawList.addCenteredText(assets.ui, "Start", {stepperX(first.start), labelsY},
                           type::caption, muted);
  drawList.addCenteredText(assets.ui, "Ring", {stepperX(first.ring), labelsY},
                           type::caption, muted);

  for (std::size_t i = 0; i < count; ++i) {
    const auto &row = layout.rotorRows[i];
    drawList.addCenteredText(assets.mono, rotorNames[i],
                             {x + 19, center(row.bounds).y}, 18);
    drawList.addBox(row.liveWindow, well, border, 0.22F);
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
  drawList.addLine({x, layout.plugboardTop - 8},
                   {right(first.bounds), layout.plugboardTop - 8}, 1, border);
}

void drawPlugboard(const Session &session, const Layout &layout,
                   const Assets &assets, Vector2 cursor, DrawList &drawList) {
  const float x = layout.clearPlugboard.x;
  const float y = layout.plugboardTop;
  const auto &pairs = session.plugPairs();
  const auto selected = session.selectedPlug();
  drawList.addText(assets.ui, "Plugboard", {x, y}, type::section);
  rightText(assets.mono, std::to_string(pairs.size()) + " / 10",
            right(layout.reset), y + 12, type::control, muted, drawList);

  std::string leads;
  for (const auto &[first, second] : pairs) {
    if (!leads.empty()) {
      leads += " ";
    }
    leads += letterAt(first) + "–" + letterAt(second);
    const auto a = layout.plugCenters[idx(first)];
    const auto b = layout.plugCenters[idx(second)];
    if (checkCollisionPointCircle(cursor, a, layout.plugRadius) ||
        checkCollisionPointCircle(cursor, b, layout.plugRadius)) {
      drawList.addLine(a, b, 2, wireLit);
    }
  }

  if (selected) {
    drawList.addText(assets.ui, "Connect " + letterAt(*selected) + " to another socket",
                     {x, y + 34}, type::control, amber);
  } else if (leads.empty()) {
    drawList.addText(assets.ui, "Click two sockets to connect them", {x, y + 34},
                     type::control, muted);
  } else {
    drawList.addText(assets.mono, leads, {x, y + 34}, type::caption, amber);
  }

  for (int i = 0; i < 26; ++i) {
    const Vector2 point = layout.plugCenters[idx(i)];
    const float radius = layout.plugRadius;
    const bool connected = session.isPlugConnected(i);
    const bool active = selected == i;
    const bool over = checkCollisionPointCircle(cursor, point, radius);
    const Color fill = active ? amber : (connected ? amberLow : raised);
    drawList.addCircle(point, radius,
                       active || over || connected ? amber : keyEdge);
    drawList.addCircle(point, radius - 1, fill);
    drawList.addCenteredText(assets.mono, letterAt(i), point, type::body,
                             active ? well : foreground);
  }
  button(assets.ui, layout.clearPlugboard, "Clear leads", cursor, drawList,
         !pairs.empty() || selected.has_value());
  button(assets.ui, layout.reset, "Reset machine", cursor, drawList);
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
                  << std::endl;
      }
      return true;
    }
  }
  return false;
}

Assets loadAssets(VulkanRenderer &renderer, const std::string &assetDir) {
  const std::string root =
      (assetDir.empty() ? resolveAssetDirectory() : assetDir) + "/";

  Assets assets{};
  const bool titleOk = loadFontWithFallback(
      assets.title, root, "InstrumentSerif-Regular.ttf", 64);
  const bool uiOk = loadFontWithFallback(
      assets.ui, root, "Montserrat-Medium.ttf", 48, controlGlyphs);
  const bool monoOk = loadFontWithFallback(
      assets.mono, root, "JetBrainsMono-Medium.ttf", 44, controlGlyphs);

  if (!titleOk && !uiOk && !monoOk) {
    std::cerr << "Fatal: No font faces could be loaded from " << root
              << std::endl;
    assets.fontsLoaded = false;
    return assets;
  }
  assets.fontsLoaded = true;

  if (titleOk && !assets.title.bitmap().empty()) {
    assets.title.setTextureId(renderer.createTexture(
        assets.title.bitmap().data(), assets.title.atlasWidth(),
        assets.title.atlasHeight(), false, true));
  }
  if (uiOk && !assets.ui.bitmap().empty()) {
    assets.ui.setTextureId(renderer.createTexture(
        assets.ui.bitmap().data(), assets.ui.atlasWidth(),
        assets.ui.atlasHeight(), false, true));
  }
  if (monoOk && !assets.mono.bitmap().empty()) {
    assets.mono.setTextureId(renderer.createTexture(
        assets.mono.bitmap().data(), assets.mono.atlasWidth(),
        assets.mono.atlasHeight(), false, true));
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

void unloadAssets(VulkanRenderer &renderer, const Assets &assets) {
  if (assets.title.textureId() != 0) {
    renderer.destroyTexture(assets.title.textureId());
  }
  if (assets.ui.textureId() != 0) {
    renderer.destroyTexture(assets.ui.textureId());
  }
  if (assets.mono.textureId() != 0) {
    renderer.destroyTexture(assets.mono.textureId());
  }
  if (assets.photoTextureId != 0) {
    renderer.destroyTexture(assets.photoTextureId);
  }
}

void drawWorkbench(const Session &session, const Layout &layout,
                   const Assets &assets, const InterfaceState &state,
                   Vector2 cursor, DrawList &drawList, int screenWidth,
                   int screenHeight) {
  const Vector2 controls = state.photoOpen ? Vector2{-1.0F, -1.0F} : cursor;
  const float left = layout.content.x;
  const float edge = right(layout.content);

  drawList.addText(assets.title, "Enigma Workbench", {left, 12}, type::title);
  std::string configuration = "Rotors ";
  for (std::size_t i = 0; i < session.rotorCount(); ++i) {
    if (i != 0) {
      configuration += ", ";
    }
    configuration += rotorNames[i];
  }
  configuration += "  ·  Reflector B";
  drawList.addText(assets.ui, configuration, {left + 1, 62}, type::control,
                   muted);
  drawPhotoButton(layout, assets, controls, drawList);
  drawList.addLine({left, 83}, {edge, 83}, 1, border);

  drawMessage(session, layout, assets, controls, drawList);
  drawKeyboard(session, layout, assets, controls, drawList);
  drawList.addBox(layout.machine, panel);
  drawSignal(session, layout.signal, assets, drawList);
  drawRotors(session, layout, assets, controls, drawList);
  drawPlugboard(session, layout, assets, controls, drawList);

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

  if (state.photoOpen) {
    drawPhotoViewer(layout, assets, cursor, drawList, screenWidth,
                    screenHeight);
  }
}

} // namespace workbench
