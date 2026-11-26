#include "Layout.hpp"

#include "Charactermap.hpp"

#include <algorithm>
#include <string_view>

namespace workbench {
namespace {

Stepper makeStepper(float x, float y, float width) {
  constexpr float button = 28;
  return {{x, y, button, button}, {x + width - button, y, button, button}};
}

void placeKeyboard(Layout &layout) {
  constexpr std::array<std::string_view, 3> rows{"QWERTZUIO", "ASDFGHJK",
                                                 "PYXCVBNML"};
  const auto &bounds = layout.keyboard;
  const float stepX = std::min(83.0F, (bounds.width - 44) / 9);
  const float stepY = std::min(83.0F, (bounds.height - 95) / 3);
  const float keyWidth = std::min(68.0F, stepX - 9);
  const float keyHeight = std::min(64.0F, stepY - 11);

  const float centerX = bounds.x + bounds.width / 2;
  const float centerY = bounds.y + (bounds.height + 49) / 2;
  for (std::size_t row = 0; row < rows.size(); ++row) {
    const float rowWidth = static_cast<float>(rows[row].size() - 1) * stepX;
    for (std::size_t column = 0; column < rows[row].size(); ++column) {
      layout.keys[idx(charToIndex(rows[row][column]))] = {
          centerX - rowWidth / 2 + static_cast<float>(column) * stepX -
              keyWidth / 2,
          centerY + (static_cast<float>(row) - 1) * stepY - keyHeight / 2,
          keyWidth, keyHeight};
    }
  }
}

void placeMachineControls(Layout &layout, std::size_t rotorCount,
                          float extraHeight, float addedRotors) {
  const auto &bounds = layout.machine;
  const float x = bounds.x + 16;
  const float width = bounds.width - 32;
  const float count = static_cast<float>(rotorCount);
  const float firstRow = layout.rotorsTop + 48;
  const float rowHeight = 34 + extraHeight * 0.42F / count - 3 * addedRotors;
  const float rowStep = rowHeight + 3;
  layout.rotorCountControl =
      makeStepper(x + width - 106, layout.rotorsTop - 2, 106);
  for (std::size_t i = 0; i < rotorCount; ++i) {
    const float y = firstRow + static_cast<float>(i) * rowStep;
    layout.rotorRows[i] = {
        {x, y, width, rowHeight},
        makeStepper(x + width - 195, y + (rowHeight - 28) / 2, 88),
        makeStepper(x + width - 91, y + (rowHeight - 28) / 2, 88),
        {x + 48, y + (rowHeight - 30) / 2, 34, 30}};
  }

  layout.plugboardTop = firstRow + count * rowStep + 13 - 1.5F * addedRotors;
  const float columnWidth = width / 9;
  const float socketRowStep = 31 + extraHeight * 0.15F - addedRotors;
  layout.plugRadius = std::min(13.5F, columnWidth * 0.43F);
  for (int i = 0; i < 26; ++i) {
    const int row = i / 9;
    layout.plugCenters[idx(i)] = {
        x + (static_cast<float>(i % 9) + 0.5F) * columnWidth,
        layout.plugboardTop + 75 + static_cast<float>(row) * socketRowStep};
  }

  const float buttonY = layout.plugboardTop + 93 + 2 * socketRowStep;
  const float buttonWidth = (width - 10) / 2;
  layout.clearPlugboard = {x, buttonY, buttonWidth, 33};
  layout.reset = {x + buttonWidth + 10, buttonY, buttonWidth, 33};
}

} // namespace

Layout makeLayout(int screenWidth, int screenHeight, std::size_t rotorCount) {
  rotorCount = std::clamp(rotorCount, minimumRotorCount, maximumRotorCount);
  Layout layout{};
  const float width = static_cast<float>(screenWidth);
  const float height = static_cast<float>(screenHeight);
  const float contentWidth = std::min(width - 48.0F, 1500.0F);
  const float left = (width - contentWidth) / 2;
  constexpr float gap = 16;
  constexpr float top = 96;
  const float availableHeight = height - top - 45;
  const float sideWidth = std::clamp(contentWidth * 0.29F, 340.0F, 392.0F);
  const float mainWidth = contentWidth - gap - sideWidth;
  constexpr float messageHeight = 229;
  const float keyboardHeight = availableHeight - messageHeight - gap;

  layout.content = {left, top, contentWidth, availableHeight};
  layout.message = {left, top, mainWidth, messageHeight};
  layout.keyboard = {left, top + messageHeight + gap, mainWidth,
                     keyboardHeight};
  layout.machine = {left + mainWidth + gap, top, sideWidth, availableHeight};
  const float extraHeight = std::clamp(height - minimumHeight, 0.0F, 160.0F);
  const float addedRotors = static_cast<float>(rotorCount - minimumRotorCount);
  layout.signal = {layout.machine.x + 16, top + 13, sideWidth - 32,
                   161 + extraHeight * 0.30F - 8 * addedRotors};

  const float right = left + mainWidth - 16;
  layout.copy = {right - 143, top + 12, 143, 33};
  layout.clear = {layout.copy.x - 73, top + 12, 63, 33};
  layout.paste = {layout.clear.x - 74, top + 12, 64, 33};

  layout.openPhoto = {left + contentWidth - 174, 25, 174, 46};
  layout.photo = {layout.openPhoto.x + 6, layout.openPhoto.y + 5, 43, 36};
  layout.rotorsTop = layout.signal.y + layout.signal.height + 10;
  const float viewerHeight = height - 48;
  const float viewerWidth = std::min(586.0F, viewerHeight * 0.80F);
  layout.photoViewer = {(width - viewerWidth) / 2, 24, viewerWidth,
                        viewerHeight};
  layout.closePhoto = {layout.photoViewer.x + viewerWidth - 76, 37, 62, 30};

  placeKeyboard(layout);
  placeMachineControls(layout, rotorCount, extraHeight, addedRotors);
  layout.machine.height = std::min(
      availableHeight, layout.reset.y + layout.reset.height + 24 - top);
  return layout;
}

} // namespace workbench
