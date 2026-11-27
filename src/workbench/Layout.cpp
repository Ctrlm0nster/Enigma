#include "Layout.hpp"

#include "Charactermap.hpp"

#include <algorithm>
#include <string_view>

namespace workbench {
namespace {

constexpr float toolbarGap = 10;
constexpr float toolbarHeight = 38;
constexpr float photoWidth = 166;
constexpr float presetRowHeight = 66;

Stepper makeStepper(float x, float y, float width) {
  constexpr float button = 28;
  return {{x, y, button, button}, {x + width - button, y, button, button}};
}

void placeTypewriter(Layout &layout) {
  constexpr std::array<std::string_view, 3> rows{"QWERTZUIO", "ASDFGHJK",
                                                 "PYXCVBNML"};
  constexpr float header = 36;
  constexpr float footer = 12;
  const float centerX = center(layout.keyboard).x;
  const float stepX = std::min(66.0F, (layout.keyboard.width - 48) / 9);
  const Rectangle lamps = layout.lampboard;
  const Rectangle keys = layout.keyboard;
  const float lampStep = std::min(40.0F, (lamps.height - header - footer) / 3);
  const float keyStep = std::min(50.0F, (keys.height - header - footer) / 3);
  const float lampY = center(lamps).y + (header - footer) / 2;
  const float keyY = center(keys).y + (header - footer) / 2;
  const float keySize = std::min(44.0F, keyStep - 3);
  layout.lampRadius = std::min(15.5F, lampStep * 0.40F);
  for (std::size_t row = 0; row < rows.size(); ++row) {
    const float rowWidth = static_cast<float>(rows[row].size() - 1) * stepX;
    const float rowOffset = static_cast<float>(row) - 1;
    for (std::size_t column = 0; column < rows[row].size(); ++column) {
      const auto letter = idx(charToIndex(rows[row][column]));
      const float x = centerX - rowWidth * 0.5F + static_cast<float>(column) * stepX;
      layout.lampCenters[letter] = {x, lampY + rowOffset * lampStep};
      layout.keys[letter] = {x - keySize * 0.5F,
                             keyY + rowOffset * keyStep - keySize * 0.5F,
                             keySize, keySize};
    }
  }
}

void placeRotorControls(Layout &layout, std::size_t count, float rowHeight) {
  const auto bounds = insetBy(layout.rotors, 16);
  const float headerY = layout.rotors.y + 12;
  layout.reflectorButton = {right(bounds) - 218, headerY, 116, 28};
  layout.rotorCountControl = makeStepper(right(bounds) - 92, headerY, 92);

  constexpr float modelWidth = 62;
  constexpr float liveWidth = 36;
  constexpr float stepperWidth = 84;

  for (std::size_t i = 0; i < count; ++i) {
    const float y = layout.rotors.y + 68 + static_cast<float>(i) * rowHeight;
    const float btnY = y + (rowHeight - 28) * 0.5F;
    layout.rotorRows[i] = {
        .bounds = {bounds.x, y, bounds.width, rowHeight},
        .modelButton = {bounds.x, btnY, modelWidth, 28},
        .start = makeStepper(right(bounds) - 2 * stepperWidth - 10, btnY,
                             stepperWidth),
        .ring = makeStepper(right(bounds) - stepperWidth, btnY, stepperWidth),
        .liveWindow = {bounds.x + modelWidth + 12, btnY, liveWidth, 28}};
  }
}

void placePlugboard(Layout &layout) {
  const auto bounds = insetBy(layout.plugboard, 16);
  const float columnWidth = bounds.width / 9;
  const float buttonY = bottom(bounds) - 30;
  const float firstSocketY = layout.plugboard.y + 72;
  const float lastSocketY = buttonY - 22;
  const float socketRowStep = std::min(46.0F, (lastSocketY - firstSocketY) / 2);
  const float middleRow = (firstSocketY + lastSocketY) / 2;
  layout.plugRadius = std::min(14.0F, columnWidth * 0.42F);
  for (int i = 0; i < 26; ++i) {
    const int row = i / 9;
    const float column = static_cast<float>(i % 9) + (row == 2 ? 1.0F : 0.5F);
    layout.plugCenters[idx(i)] = {bounds.x + column * columnWidth,
                                  middleRow + static_cast<float>(row - 1) * socketRowStep};
  }

  const float buttonWidth = (bounds.width - 10) / 2;
  layout.clearPlugboard = {bounds.x, buttonY, buttonWidth, 30};
  layout.reset = {right(layout.clearPlugboard) + 10, buttonY, buttonWidth, 30};
}

} // namespace

Rectangle Layout::presetRow(std::size_t index) const {
  return {presetsMenu.x + 8, presetsMenu.y + 34 + static_cast<float>(index) * presetRowHeight,
          presetsMenu.width - 16, presetRowHeight - 4};
}

Rectangle decipherLauncher(Rectangle content) {
  constexpr float width = 106;
  return {right(content) - photoWidth - toolbarGap - width, 24, width,
          toolbarHeight};
}

Layout makeLayout(int screenWidth, int screenHeight, std::size_t rotorCount) {
  rotorCount = std::clamp(rotorCount, minimumRotorCount, maximumRotorCount);
  Layout layout{};
  layout.screenWidth = screenWidth;
  layout.screenHeight = screenHeight;
  const float width = static_cast<float>(screenWidth);
  const float height = static_cast<float>(screenHeight);
  const float contentWidth = std::min(width - 48.0F, 1500.0F);
  const float left = (width - contentWidth) / 2;
  constexpr float gap = 12;
  constexpr float top = 88;
  constexpr float signalHeight = 102;
  const float availableHeight = height - top - 36;
  const float consoleHeight = availableHeight - signalHeight - gap;
  const float sideWidth = std::clamp(contentWidth * 0.31F, 344.0F, 410.0F);
  const float mainWidth = contentWidth - gap - sideWidth;
  constexpr float messageHeight = 180;
  const float extraHeight = std::clamp(height - minimumHeight, 0.0F, 200.0F);
  const float lampboardHeight = 130 + extraHeight * 0.35F;
  const float keyboardHeight = consoleHeight - messageHeight - lampboardHeight - 2 * gap;
  const float rotorRowHeight = 32 + extraHeight * 0.06F;
  const float rotorHeight = 82 + static_cast<float>(rotorCount) * rotorRowHeight;

  layout.content = {left, top, contentWidth, availableHeight};
  layout.message = {left, top, mainWidth, messageHeight};
  layout.lampboard = {left, top + messageHeight + gap, mainWidth, lampboardHeight};
  layout.keyboard = {left, bottom(layout.lampboard) + gap, mainWidth, keyboardHeight};
  layout.machine = {left + mainWidth + gap, top, sideWidth, consoleHeight};
  layout.rotors = {layout.machine.x, top, sideWidth, rotorHeight};
  layout.plugboard = {layout.machine.x, bottom(layout.rotors) + gap, sideWidth,
                      consoleHeight - rotorHeight - gap};
  layout.signal = {left, bottom(layout.machine) + gap, contentWidth, signalHeight};

  const float messageEdge = right(layout.message) - 16;
  layout.copy = {messageEdge - 143.0F, top + 9, 143.0F, 32};
  layout.clear = {layout.copy.x - 73, layout.copy.y, 63, layout.copy.height};
  layout.paste = {layout.clear.x - 74, layout.copy.y, 64, layout.copy.height};

  const auto launcher = decipherLauncher(layout.content);
  layout.openPhoto = {right(launcher) + toolbarGap, launcher.y, photoWidth, toolbarHeight};
  layout.photo = {layout.openPhoto.x + 6, launcher.y + 4, 36, toolbarHeight - 8};
  layout.presetsButton = {launcher.x - toolbarGap - 100, launcher.y, 100, toolbarHeight};
  layout.soundToggle = {layout.presetsButton.x - toolbarGap - 112, launcher.y, 112, toolbarHeight};
  constexpr float menuWidth = 400;
  const auto presetCount = static_cast<float>(enigma::historicalPresets().size());
  layout.presetsMenu = {right(layout.presetsButton) - menuWidth, bottom(layout.presetsButton) + 8,
                        menuWidth, 42 + presetCount * presetRowHeight};
  const float viewerHeight = height - 48;
  const float viewerWidth = std::min(586.0F, viewerHeight * 0.80F);
  layout.photoViewer = {(width - viewerWidth) / 2, 24, viewerWidth,
                        viewerHeight};
  layout.closePhoto = {layout.photoViewer.x + viewerWidth - 76, 37, 62, 30};

  placeTypewriter(layout);
  placeRotorControls(layout, rotorCount, rotorRowHeight);
  placePlugboard(layout);
  return layout;
}

} // namespace workbench
