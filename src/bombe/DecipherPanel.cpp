#include "bombe/DecipherPanel.hpp"

#include "Charactermap.hpp"
#include "bombe/DecipherInput.hpp"
#include "workbench/Input.hpp"
#include "workbench/Session.hpp"
#include "workbench/Typography.hpp"
#include "workbench/View.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>

namespace bombe {
namespace {

using namespace workbench;

struct PanelLayout {
  Rectangle body;
  Rectangle launcher;
  Rectangle form;
  Rectangle results;
  Rectangle field(float y, float height) const {
    return {form.x + 16, y, form.width - 32, height};
  }
  Rectangle back() const { return {right(body) - 164, 28, 164, 36}; }
  Rectangle knownTab() const { return {body.x, 86, 143, 36}; }
  Rectangle searchTab() const { return {body.x + 151, 86, 143, 36}; }
  Rectangle useSettings() const { return {right(form) - 174, 148, 158, 28}; }
  Rectangle ciphertext() const { return field(188, 109); }
  Rectangle crib() const { return field(326, 83); }
  Rectangle start() const { return field(326, 34); }
  Rectangle rings(bool search) const {
    auto box = field(search ? 438 : 391, 34);
    if (search) {
      box.width *= 0.62F;
    }
    return box;
  }
  Rectangle offset() const {
    const auto ringBox = rings(true);
    return {right(ringBox) + 10, ringBox.y, right(form) - right(ringBox) - 26,
            ringBox.height};
  }
  Rectangle boardMode(int i) const {
    const auto row = field(489, 31);
    const float width = (row.width - 16) / 3;
    return {row.x + static_cast<float>(i) * (width + 8), row.y, width,
            row.height};
  }
  Rectangle leads(bool search) const { return field(search ? 550 : 459, 34); }
  Rectangle run() const { return {form.x + 16, bottom(form) - 53, 128, 35}; }
  Rectangle cancel() const { return {form.x + 153, run().y, 92, 35}; }
  Rectangle previous() const { return {results.x + 16, 278, 98, 32}; }
  Rectangle next() const { return {right(results) - 110, 278, 94, 32}; }
  Rectangle preview(bool search) const {
    const float y = search ? 467.0F : 225.0F;
    return {results.x + 16, y, results.width - 32, bottom(results) - y - 62};
  }
  Rectangle copy() const { return {results.x + 16, run().y, 146, 35}; }
  Rectangle loadKey() const { return {right(copy()) + 10, run().y, 118, 35}; }
};

PanelLayout panelLayout(int width, int height) {
  const float available = std::min(static_cast<float>(width) - 48, 1500.0F);
  const float left = (static_cast<float>(width) - available) / 2;
  const float formWidth = available * 0.46F;
  const float bodyHeight = static_cast<float>(height) - 182;
  return {{left, 0, available, static_cast<float>(height)},
          {left + available - 296, 25, 112, 46},
          {left, 136, formWidth, bodyHeight},
          {left + formWidth + 16, 136, available - formWidth - 16, bodyHeight}};
}

constexpr std::array boardModes{PlugboardMode::Unknown, PlugboardMode::Partial,
                                PlugboardMode::Known};
constexpr std::array boardLabels{"Unknown", "Known leads", "Known board"};

void button(DrawList &draw, const FontAtlas &font, Rectangle bounds,
            std::string_view text, Vector2 cursor, bool enabled = true,
            bool primary = false) {
  const bool hover = enabled && checkCollisionPointRec(cursor, bounds);
  const auto color = primary && enabled ? (hover ? amberHover : amber)
                                        : (hover ? hoverFill : raised);
  draw.addBox(bounds, color, primary && enabled ? color : border, 0.24F);
  const Color textColor = primary && enabled ? well : (enabled ? foreground : muted);
  draw.addCenteredText(font, text, center(bounds), type::control, textColor);
}

void tape(DrawList &draw, const FontAtlas &mono, Rectangle bounds,
          std::string_view text, std::size_t highlight = 0,
          std::size_t highlightLength = 0, bool tail = false) {
  constexpr float fontSize = 15;
  constexpr float padding = 10;
  const float lineAdvance = mono.lineAdvance(fontSize);
  const float advance = mono.measure("M", fontSize).x;
  const Rectangle textArea = insetBy(bounds, padding);
  if (advance <= 0 || textArea.height < fontSize || textArea.width <= 0) {
    return;
  }
  const auto groups = std::max(
      std::size_t{1},
      static_cast<std::size_t>((textArea.width + advance) / (6 * advance)));
  const auto perLine = groups * 5;
  const auto rows = 1 + static_cast<std::size_t>((textArea.height - fontSize) /
                                               lineAdvance);
  const auto totalRows = (text.size() + perLine - 1) / perLine;
  const auto firstRow = tail && totalRows > rows ? totalRows - rows : 0;
  draw.setScissor(insetBy(bounds, 2));
  for (std::size_t i = firstRow * perLine; i < text.size(); ++i) {
    const auto row = i / perLine - firstRow;
    if (row == rows) {
      break;
    }
    const auto column = i % perLine;
    const bool inCrib = i >= highlight && i - highlight < highlightLength;
    draw.addText(
        mono, text.substr(i, 1),
        {textArea.x + static_cast<float>(column + column / 5) * advance,
         textArea.y + static_cast<float>(row) * lineAdvance},
        fontSize, inCrib ? amber : foreground);
  }
  draw.resetScissor();
}

} // namespace

void DecipherPanel::copySettings(enigma::MachineSettings settings) {
  if (!searchMode_) {
    fields_[Start] = formatStarts(settings);
    fields_[Rings] = formatRings(settings);
    fields_[Leads] = formatLeads(settings.plugPairs);
    status_ = "Workbench key copied";
  } else {
    settings.rotorCount = rotorCount;
    fields_[SearchRings] = formatRings(settings);
    fields_[SearchLeads] = formatLeads(settings.plugPairs);
    boardMode_ = PlugboardMode::Known;
    status_ = "First three rings and plugboard copied";
  }
}

void DecipherPanel::edit(std::string_view text) {
  auto value = replace_ ? std::string{} : fields_[focus_];
  const bool message = focus_ == Ciphertext || focus_ == Crib;
  const bool letters = message || focus_ == Start;
  const bool leads = focus_ == Leads || focus_ == SearchLeads;
  const auto limit = message           ? enigma::messageLimit
                     : focus_ == Start ? enigma::maximumRotorCount
                                       : 48;
  for (const char character : text) {
    const char letter = enigma::normalizeLetter(character);
    const bool separator = character == ' ' || character == ',';
    const bool digit = character >= '0' && character <= '9';
    char accepted = 0;
    if (letters) {
      accepted = letter;
    } else if (leads) {
      accepted = separator ? character : letter;
    } else if (digit || (focus_ != Offset && separator)) {
      accepted = character;
    }
    if (accepted != 0) {
      if (value.size() == limit) {
        status_ = message ? "The message accepts up to 240 letters"
                          : "This field is full";
        return;
      }
      value.push_back(accepted);
    }
  }
  if (!value.empty() || !replace_) {
    fields_[focus_] = std::move(value);
    replace_ = false;
    status_.clear();
  }
}

void DecipherPanel::run() {
  if (running_) {
    return;
  }
  try {
    if (!searchMode_) {
      knownPlaintext_.clear();
      const auto settings =
          parseSettings(fields_[Start], fields_[Rings], fields_[Leads]);
      knownPlaintext_ = decipher(fields_[Ciphertext], settings);
      status_ = "Plaintext ready";
      return;
    }

    progress_ = {};
    result_.reset();
    selected_ = 0;
    startTime_ = std::chrono::steady_clock::now();
    searchDurationSeconds_ = 0.0;
    auto request = parseSearch(
        fields_[Ciphertext], fields_[Crib], fields_[SearchRings],
        fields_[Offset], boardMode_,
        boardMode_ == PlugboardMode::Unknown ? "" : fields_[SearchLeads]);
    cribLength_ = fields_[Crib].size();
    status_ = "Preparing search";
    worker_ = std::jthread([this, request = std::move(request)](
                               std::stop_token stop) {
      try {
        auto result = search(request, {}, stop, [this](const auto &progress) {
          std::scoped_lock lock(mailboxMutex_);
          pendingProgress_ = progress;
        });
        std::scoped_lock lock(mailboxMutex_);
        pendingResult_ = std::move(result);
      } catch (const std::exception &error) {
        std::scoped_lock lock(mailboxMutex_);
        pendingError_ = error.what();
      }
    });
    running_ = true;
  } catch (const std::exception &error) {
    status_ = error.what();
  }
}

bool DecipherPanel::update() {
  if (!running_) {
    return false;
  }
  std::scoped_lock lock(mailboxMutex_);
  bool changed = false;
  if (pendingProgress_) {
    progress_ = *pendingProgress_;
    pendingProgress_.reset();
    if (progress_.work != 0 && !worker_.get_stop_token().stop_requested()) {
      status_ = "Searching rotor starts and plugboard";
    }
    changed = true;
  }
  if (pendingResult_) {
    result_ = std::exchange(pendingResult_, std::nullopt);
    running_ = false;
    searchDurationSeconds_ = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - startTime_).count();
    status_ =
        result_->status == SearchStatus::Complete && result_->candidates.empty()
            ? "No match in this search"
            : statusText(result_->status);
    changed = true;
  } else if (!pendingError_.empty()) {
    running_ = false;
    searchDurationSeconds_ = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - startTime_).count();
    status_ = std::exchange(pendingError_, {});
    changed = true;
  }
  return changed && open_;
}

std::string_view DecipherPanel::plaintext() const {
  if (!searchMode_) {
    return knownPlaintext_;
  }
  return result_ && !result_->candidates.empty()
             ? std::string_view(result_->candidates[selected_].plaintext)
             : std::string_view{};
}

std::optional<bool> DecipherPanel::handle(const InputEvent &event,
                                          const PlatformContext &platform,
                                          Session &session, int width,
                                          int height) {
  const auto layout = panelLayout(width, height);
  const bool click = event.type == EventType::MouseButton &&
                     event.action == GLFW_PRESS &&
                     event.button == GLFW_MOUSE_BUTTON_LEFT;
  const auto hit = [&](Rectangle bounds) {
    return checkCollisionPointRec(event.cursor, bounds);
  };
  if (!open_) {
    if (click && hit(layout.launcher)) {
      open_ = true;
      return true;
    }
    return std::nullopt;
  }
  if (event.type == EventType::WindowClose ||
      event.type == EventType::WindowResize) {
    return std::nullopt;
  }

  const bool key = event.type == EventType::Key && event.action != GLFW_RELEASE;
  const bool press = key && event.action == GLFW_PRESS;
  const bool control = (event.mods & GLFW_MOD_CONTROL) != 0;
  if ((click && hit(layout.back())) ||
      (press && event.key == GLFW_KEY_ESCAPE)) {
    worker_.request_stop();
    open_ = false;
    return true;
  }
  if (press && control && event.key == GLFW_KEY_V) {
    edit(platform.getClipboard());
    return true;
  }
  if ((press && control && event.key == GLFW_KEY_C) ||
      (click && hit(layout.copy()))) {
    if (!plaintext().empty()) {
      platform.setClipboard(plaintext());
      status_ = "Plaintext copied";
    }
    return true;
  }
  if (press && control && event.key == GLFW_KEY_A) {
    replace_ = true;
    return true;
  }
  if (key && event.key == GLFW_KEY_BACKSPACE) {
    if (replace_) {
      fields_[focus_].clear();
      replace_ = false;
    } else if (!fields_[focus_].empty()) {
      fields_[focus_].pop_back();
    }
    return true;
  }
  if (event.type == EventType::Char && !control && event.codepoint < 128) {
    const char character = static_cast<char>(event.codepoint);
    edit(std::string_view(&character, 1));
    return true;
  }
  if ((click && hit(layout.run())) || (press && event.key == GLFW_KEY_ENTER)) {
    run();
    return true;
  }
  if (!click) {
    return event.type == EventType::CursorPos;
  }

  if (hit(layout.cancel()) && running_) {
    worker_.request_stop();
    status_ = "Cancelling search";
  } else if (!running_ && (hit(layout.knownTab()) || hit(layout.searchTab()))) {
    searchMode_ = hit(layout.searchTab());
    focus_ = Ciphertext;
    replace_ = false;
    status_.clear();
  } else if (hit(layout.useSettings())) {
    copySettings(session.settings());
  } else if (searchMode_ && result_ && !result_->candidates.empty() &&
             hit(layout.loadKey())) {
    try {
      session.applySettings(result_->candidates[selected_].settings);
      status_ = "Preview key loaded in Workbench";
    } catch (const std::exception &error) {
      status_ = error.what();
    }
  } else if (searchMode_ && result_ && hit(layout.previous()) &&
             selected_ > 0) {
    --selected_;
  } else if (searchMode_ && result_ && hit(layout.next()) &&
             selected_ + 1 < result_->candidates.size()) {
    ++selected_;
  } else {
    const auto select = [&](Field field, Rectangle bounds) {
      if (hit(bounds)) {
        focus_ = field;
        replace_ = false;
      }
    };
    select(Ciphertext, layout.ciphertext());
    select(searchMode_ ? SearchRings : Rings, layout.rings(searchMode_));
    if (searchMode_) {
      select(Crib, layout.crib());
      select(Offset, layout.offset());
      for (std::size_t i = 0; i < boardModes.size(); ++i) {
        if (hit(layout.boardMode(static_cast<int>(i)))) {
          boardMode_ = boardModes[i];
          if (boardMode_ == PlugboardMode::Unknown && focus_ == SearchLeads) {
            focus_ = Crib;
          }
        }
      }
      if (boardMode_ != PlugboardMode::Unknown) {
        select(SearchLeads, layout.leads(true));
      }
    } else {
      select(Start, layout.start());
      select(Leads, layout.leads(false));
    }
  }
  return true;
}

void DecipherPanel::draw(DrawList &draw, const Assets &assets, Vector2 cursor,
                         int width, int height) const {
  const auto layout = panelLayout(width, height);
  const auto &ui = assets.ui;
  const auto &mono = assets.mono;
  if (!open_) {
    button(draw, ui, layout.launcher, "Decipher", cursor);
    return;
  }

  draw.addRectangle(
      {0, 0, static_cast<float>(width), static_cast<float>(height)},
      background);
  draw.addText(assets.title, "Decipher", {layout.body.x, 12}, type::title);
  button(draw, ui, layout.back(), "Back to Workbench", cursor);
  draw.addLine({layout.body.x, 75}, {right(layout.body), 75}, 1, border);
  button(draw, ui, layout.knownTab(), "Known settings", cursor, !running_,
         !searchMode_);
  button(draw, ui, layout.searchTab(), "Bombe search", cursor, !running_,
         searchMode_);
  draw.addBox(layout.form, panel);
  draw.addBox(layout.results, panel);
  button(draw, ui, layout.useSettings(), "Use Workbench key", cursor);

  const auto field = [&](Field id, Rectangle bounds, std::string_view label,
                         bool enabled = true,
                         std::string_view placeholder = {}) {
    const bool focused = focus_ == id && enabled;
    draw.addText(ui, label, {bounds.x, bounds.y - 22}, type::control,
                 enabled ? foreground : muted);
    draw.addBox(bounds, focused && replace_ ? amberWell : well,
                focused ? amber : border, 0.15F);
    if (id == Ciphertext || id == Crib) {
      tape(draw, mono, bounds, fields_[id], 0, 0, true);
    } else if (enabled) {
      const float textTop = mono.centeredLineTop(center(bounds).y, type::body);
      const bool empty = fields_[id].empty();
      draw.setScissor(insetBy(bounds, 5));
      draw.addText(mono, empty ? placeholder : std::string_view(fields_[id]),
                   {bounds.x + 10, textTop}, type::body,
                   empty ? muted : foreground);
      draw.resetScissor();
    }
  };
  field(Ciphertext, layout.ciphertext(), "Ciphertext");
  if (searchMode_) {
    field(Crib, layout.crib(), "Crib (expected plaintext)");
    field(SearchRings, layout.rings(true), "I II III rings (01-26)");
    field(Offset, layout.offset(), "Crib position", true, "Auto");
    for (std::size_t i = 0; i < boardModes.size(); ++i) {
      button(draw, ui, layout.boardMode(static_cast<int>(i)), boardLabels[i],
             cursor, true, boardMode_ == boardModes[i]);
    }
    field(SearchLeads, layout.leads(true), "Plugboard pairs (AZ BY)",
          boardMode_ != PlugboardMode::Unknown);
  } else {
    field(Start, layout.start(), "Starting windows (3-5 letters)");
    field(Rings, layout.rings(false), "Rings (01-26, left to right)");
    field(Leads, layout.leads(false), "Plugboard pairs (AZ BY)");
  }
  button(draw, ui, layout.run(), searchMode_ ? "Run search" : "Decipher",
         cursor, !running_, true);
  if (searchMode_) {
    button(draw, ui, layout.cancel(), "Cancel", cursor, running_);
  }
  const float resultX = layout.results.x + 16;
  draw.addText(ui, searchMode_ ? "Bombe results" : "Plaintext", {resultX, 156},
               type::section);
  draw.addText(ui, status_, {resultX, 191}, type::body, amber);
  if (searchMode_) {
    draw.addText(mono,
                 std::to_string(progress_.viableAlignments) + "/" +
                     std::to_string(progress_.alignments) +
                     " viable alignments",
                 {resultX, 223}, type::control, muted);
    draw.addText(mono,
                 std::to_string(progress_.settingsChecked) +
                     " settings tested  /  " +
                     std::to_string(progress_.matches) + " crib matches",
                 {resultX, 245}, type::control, muted);

    const double elapsed = running_
        ? std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime_).count()
        : searchDurationSeconds_;
    if (elapsed > 0.001) {
      const double states = static_cast<double>(progress_.work > 0 ? progress_.work : progress_.settingsChecked);
      const double mKeys = states / (elapsed * 1'000'000.0);
      char speedBuf[96];
      std::snprintf(speedBuf, sizeof(speedBuf), "Throughput: %.2f M keys/sec (%.2fs)", mKeys, elapsed);
      draw.addText(mono, speedBuf, {resultX, 267}, type::control, amber);
    }

    // Scrambler drum bank visualization (British Turing-Welchman Bombe replica bank)
    const float drumsRight = right(layout.results) - 20;
    const float drumsWidth = 230;
    const float drumsX = drumsRight - drumsWidth;
    const float drumsY = 162;
    const float drumsHeight = 92;
    draw.addBox({drumsX, drumsY, drumsWidth, drumsHeight}, well, border, 0.12F);
    draw.addCenteredText(ui, "TURING SCRAMBLERS", {drumsX + drumsWidth * 0.5F, drumsY + 12}, type::caption, amber);

    const std::size_t step = progress_.settingsChecked;
    const std::array<Color, 3> drumColors{
        Color{60, 20, 25, 255},  // Red drum
        Color{18, 32, 60, 255},  // Blue drum
        Color{18, 50, 30, 255}   // Green drum
    };

    for (int d = 0; d < 3; ++d) {
      const float dcx = drumsX + 38.0F + static_cast<float>(d) * 77.0F;
      const float dcy = drumsY + 48.0F;
      const float drumRadius = 24.0F;

      int curOffset = 0;
      if (running_) {
        curOffset = (d == 0) ? static_cast<int>(step % 26)
                             : (d == 1) ? static_cast<int>((step / 26) % 26)
                                        : static_cast<int>((step / 676) % 26);
      } else if (result_ && !result_->candidates.empty()) {
        curOffset = result_->candidates[selected_].settings.starts[d];
      } else {
        curOffset = d * 5;
      }

      draw.addCircle({dcx, dcy}, drumRadius + 2.0F, Color{175, 145, 65, 255});
      draw.addCircle({dcx, dcy}, drumRadius + 0.5F, Color{85, 70, 30, 255});
      draw.addCircle({dcx, dcy}, drumRadius - 1.0F, drumColors[d]);

      const char centerLetter = indexToChar(curOffset);
      const char prevLetter = indexToChar((curOffset + 25) % 26);
      const char nextLetter = indexToChar((curOffset + 1) % 26);

      draw.addCenteredText(mono, std::string(1, prevLetter), {dcx, dcy - 14.0F}, 11.0F, Color{160, 160, 160, 140});
      draw.addCenteredText(mono, std::string(1, centerLetter), {dcx, dcy}, 17.0F, amber);
      draw.addCenteredText(mono, std::string(1, nextLetter), {dcx, dcy + 14.0F}, 11.0F, Color{160, 160, 160, 140});

      draw.addCenteredText(ui, enigma::rotorNames[d], {dcx, drumsY + drumsHeight - 6.0F}, type::caption, muted);
    }
  }
  std::size_t highlight = 0;
  std::size_t highlightedLength = 0;
  if (searchMode_ && result_ && !result_->candidates.empty()) {
    const auto &match = result_->candidates[selected_];
    highlight = match.cribOffset;
    highlightedLength = cribLength_;
    button(draw, ui, layout.previous(), "Previous", cursor, selected_ > 0);
    button(draw, ui, layout.next(), "Next", cursor,
           selected_ + 1 < result_->candidates.size());
    const auto matchLabelX = (right(layout.previous()) + layout.next().x) * 0.5F;
    const auto matchLabel = "Crib match " + std::to_string(selected_ + 1) + " / " +
                            std::to_string(result_->candidates.size());
    draw.addCenteredText(ui, matchLabel,
                         {matchLabelX, center(layout.previous()).y}, type::control);
    draw.addText(mono,
                 "Start " + formatStarts(match.settings) + "  /  Rings " +
                     formatRings(match.settings),
                 {resultX, 326}, type::body);
    draw.addText(mono, "Crib position " + std::to_string(match.cribOffset + 1),
                 {resultX, 350}, type::control);
    const auto leads = formatLeads(match.settings.plugPairs);
    draw.addText(mono, "Leads " + (leads.empty() ? std::string("none") : leads),
                 {resultX, 372}, type::control);
    if (!match.unresolvedSockets.empty()) {
      draw.addText(ui, "Assumed unplugged in preview", {resultX, 398}, type::control,
                   muted);
      draw.addText(mono, match.unresolvedSockets, {resultX, 420}, type::control,
                   muted);
    }
  }
  if (searchMode_) {
    draw.addText(ui, "Plaintext preview", {resultX, layout.preview(true).y - 23},
                 type::control, muted);
  }
  draw.addBox(layout.preview(searchMode_), well, border, 0.09F);
  tape(draw, mono, layout.preview(searchMode_), plaintext(), highlight,
       highlightedLength);
  button(draw, ui, layout.copy(), "Copy plaintext", cursor,
         !plaintext().empty());
  if (searchMode_ && result_ && !result_->candidates.empty()) {
    button(draw, ui, layout.loadKey(), "Load key", cursor);
  }
  draw.addText(ui, "Ctrl+V  Paste    ·    Ctrl+A  Select field    ·    Esc  Back",
               {layout.body.x, ui.centeredLineTop(static_cast<float>(height) - 22,
                                                 type::control)},
               type::control, muted);
}

} // namespace bombe
