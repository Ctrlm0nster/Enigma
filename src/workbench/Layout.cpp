#include "Layout.hpp"

#include "Charactermap.hpp"

#include <algorithm>
#include <string_view>

namespace workbench {
namespace {

Stepper makeStepper(float x, float y, float width) {
  constexpr float button = 29;
  return {{x, y, button, button}, {x + width - button, y, button, button}};
}

void placeKeyboard(Layout &layout) {
  constexpr std::array<std::string_view, 3> rows{"QWERTZUIO", "ASDFGHJK",
                                                 "PYXCVBNML"};
  const auto &bounds = layout.keyboard;
  const float stepX = std::min(76.0F, (bounds.width - 52) / 9);
  const float stepY = std::min(72.0F, (bounds.height - 76) / 3);
  layout.keyRadius = std::min({28.0F, stepX * 0.38F, stepY * 0.41F});

  const float centerX = bounds.x + bounds.width / 2;
  const float centerY = bounds.y + (bounds.height + 48) / 2;
  for (std::size_t row = 0; row < rows.size(); ++row) {
    const float rowWidth = static_cast<float>(rows[row].size() - 1) * stepX;
    for (std::size_t column = 0; column < rows[row].size(); ++column) {
      layout.keyCenters[idx(charToIndex(rows[row][column]))] = {
          centerX - rowWidth / 2 + static_cast<float>(column) * stepX,
          centerY + (static_cast<float>(row) - 1) * stepY};
    }
  }
}

void placeMachineControls(Layout &layout) {
  const auto &bounds = layout.machine;
  const float x = bounds.x + 16;
  const float width = bounds.width - 32;
  const float firstRow = bounds.y + 76;
  constexpr float rowHeight = 56;
  constexpr float rowStep = rowHeight + 8;
  for (std::size_t i = 0; i < layout.rotorRows.size(); ++i) {
    const float y = firstRow + static_cast<float>(i) * rowStep;
    layout.rotorRows[i] = {{x, y, width, rowHeight},
                           makeStepper(x + width - 202, y + 13, 91),
                           makeStepper(x + width - 101, y + 13, 91),
                           {x + 51, y + 9, 38, 38}};
  }

  layout.plugboardTop = firstRow + static_cast<float>(rotorCount) * rowStep + 6;
  const float columnWidth = width / 9;
  layout.plugRadius = std::min(14.5F, columnWidth * 0.43F);
  for (int i = 0; i < 26; ++i) {
    layout.plugCenters[idx(i)] = {
        x + (static_cast<float>(i % 9) + 0.5F) * columnWidth,
        layout.plugboardTop + 111 + static_cast<float>(i / 9) * 36};
  }

  const float buttonY = layout.plugboardTop + 227;
  const float buttonWidth = (width - 10) / 2;
  layout.clearPlugboard = {x, buttonY, buttonWidth, 36};
  layout.reset = {x + buttonWidth + 10, buttonY, buttonWidth, 36};
}

} // namespace

Layout makeLayout(int screenWidth, int screenHeight) {
  Layout layout{};
  const float width = static_cast<float>(screenWidth);
  const float height = static_cast<float>(screenHeight);
  constexpr float gap = 16;
  constexpr float top = 80;
  const float availableHeight = height - top - 42;
  const float sideWidth = std::clamp(width * 0.29F, 348.0F, 386.0F);
  const float mainWidth = width - 2 * margin - gap - sideWidth;
  const float messageHeight = 176;
  const float signalHeight = 126;
  const float keyboardHeight =
      availableHeight - messageHeight - signalHeight - 2 * gap;

  layout.message = {margin, top, mainWidth, messageHeight};
  layout.signal = {margin, top + messageHeight + gap, mainWidth, signalHeight};
  layout.keyboard = {margin, layout.signal.y + signalHeight + gap, mainWidth,
                     keyboardHeight};
  layout.machine = {margin + mainWidth + gap, top, sideWidth, availableHeight};

  const float right = layout.message.x + layout.message.width - 16;
  layout.clear = {right - 70, top + 13, 70, 34};
  layout.copy = {layout.clear.x - 121, top + 13, 111, 34};

  placeKeyboard(layout);
  placeMachineControls(layout);
  return layout;
}

} // namespace workbench
