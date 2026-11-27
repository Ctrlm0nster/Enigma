#include "bombe/DecipherPanel.hpp"

#include "Charactermap.hpp"
#include "bombe/DecipherInput.hpp"
#include "workbench/Input.hpp"
#include "workbench/Layout.hpp"
#include "workbench/Session.hpp"
#include "workbench/Typography.hpp"
#include "workbench/View.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iterator>
#include <random>

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
  Rectangle back() const { return {right(body) - 164, launcher.y, 164, launcher.height}; }
  Rectangle knownTab() const { return {body.x, 86, 143, 36}; }
  Rectangle searchTab() const { return {body.x + 151, 86, 143, 36}; }
  Rectangle useSettings() const { return {right(form) - 174, 148, 158, 28}; }
  Rectangle challenge() const { return {right(searchTab()) + 12, 86, 143, 36}; }
  Rectangle ciphertext() const { return field(188, 109); }
  Rectangle crib() const { return field(326, 64); }
  Rectangle start() const { return field(326, 34); }
  Rectangle rings(bool search) const {
    auto box = field(search ? 413 : 391, 34);
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
  Rectangle scope(int i) const {
    const auto row = field(454, 28);
    const float width = (row.width - 18) / 4;
    return {row.x + static_cast<float>(i) * (width + 6), row.y, width, row.height};
  }
  Rectangle boardMode(int i) const {
    const auto row = field(489, 31);
    const float width = (row.width - 16) / 3;
    return {row.x + static_cast<float>(i) * (width + 8), row.y, width,
            row.height};
  }
  Rectangle leads(bool search) const { return field(search ? 550 : 459, 34); }
  Rectangle run() const { return {form.x + 16, bottom(form) - 53, 122, 35}; }
  Rectangle pause() const { return {right(run()) + 8, run().y, 94, run().height}; }
  Rectangle step() const { return {right(pause()) + 8, run().y, 82, run().height}; }
  Rectangle cancel() const { return {right(step()) + 8, run().y, 90, run().height}; }
  Rectangle previous() const { return {results.x + 16, 301, 98, 30}; }
  Rectangle next() const { return {right(results) - 110, 301, 94, 30}; }
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
  const Rectangle body{left, 0, available, static_cast<float>(height)};
  return {body, decipherLauncher(body),
          {left, 136, formWidth, bodyHeight},
          {left + formWidth + 16, 136, available - formWidth - 16, bodyHeight}};
}

constexpr std::array boardModes{PlugboardMode::Unknown, PlugboardMode::Partial,
                                PlugboardMode::Known};
constexpr std::array boardLabels{"Unknown", "Known leads", "Known board"};
constexpr std::array rotorScopes{RotorScope::Fixed, RotorScope::FirstThree,
                                  RotorScope::FirstFive};
constexpr std::array rotorLabels{"I–III fixed", "I–III orders", "I–V orders"};
constexpr std::array reflectorLabels{"Refl. B", "Refl. C", "B or C"};

enum class ButtonTone { Normal, Primary, Selected };

ButtonTone selection(bool selected) {
  return selected ? ButtonTone::Selected : ButtonTone::Normal;
}

void button(DrawList &draw, const FontAtlas &font, Rectangle bounds,
            std::string_view text, Vector2 cursor, bool enabled = true,
            ButtonTone tone = ButtonTone::Normal) {
  const bool hover = enabled && checkCollisionPointRec(cursor, bounds);
  const bool highlighted = enabled && tone != ButtonTone::Normal;
  const bool lockedSelection = !enabled && tone == ButtonTone::Selected;
  const Color fill = highlighted ? (hover ? amberHover : amber)
                     : lockedSelection ? amberWell
                                       : (hover ? hoverFill : raised);
  const Color edge = highlighted ? fill : (lockedSelection ? amberLow : border);
  draw.addBox(bounds, fill, edge, 0.24F);
  const Color textColor = highlighted ? well : lockedSelection ? amber
                                               : enabled ? foreground : muted;
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
  const auto lastStartRow = totalRows > rows ? totalRows - rows : 0;
  const auto firstRow = highlightLength != 0
                            ? std::min(highlight / perLine, lastStartRow)
                            : (tail ? lastStartRow : 0);
  draw.setScissor(insetBy(bounds, 2));
  for (std::size_t i = firstRow * perLine; i < text.size(); ++i) {
    const auto row = i / perLine - firstRow;
    if (row == rows) {
      break;
    }
    const auto column = i % perLine;
    const auto groupSeparators = column / 5;
    const auto tapeColumn = column + groupSeparators;
    const bool inCrib = i >= highlight && i - highlight < highlightLength;
    draw.addText(
        mono, text.substr(i, 1),
        {textArea.x + static_cast<float>(tapeColumn) * advance,
         textArea.y + static_cast<float>(row) * lineAdvance},
        fontSize, inCrib ? amber : foreground);
  }
  draw.resetScissor();
}

} // namespace

void DecipherPanel::copySettings(enigma::MachineSettings settings) {
  clearPreview();
  if (!searchMode_) {
    fields_[Start] = formatStarts(settings);
    fields_[Rings] = formatRings(settings);
    fields_[Leads] = formatLeads(settings.plugPairs);
    knownRotors_ = settings.rotorTypes;
    knownReflector_ = settings.reflectorIndex;
    status_ = "Workbench key copied";
  } else {
    settings.rotorCount = rotorCount;
    fields_[SearchRings] = formatRings(settings);
    fields_[SearchLeads] = formatLeads(settings.plugPairs);
    boardMode_ = PlugboardMode::Known;
    status_ = "First three rings and plugboard copied";
  }
}

void DecipherPanel::loadChallenge() {
  constexpr std::string_view message =
      "MEETATTHEWEATHERSTATIONATDAWNBRINGEXTRASUPPLIESFORTHETEAM"
      "THENWAITFORTHERADIOOPERATORINTHELOOKOUTTOWER";
  auto key = parseSettings("AAA", "03 02 05", "AZ BY CR");
  std::mt19937 random(std::random_device{}());
  std::shuffle(key.rotorTypes.begin(), key.rotorTypes.begin() + rotorCount,
               random);
  std::uniform_int_distribution<int> window(0, 25);
  for (std::size_t i = 0; i < rotorCount; ++i) {
    key.starts[i] = window(random);
  }
  key.reflectorIndex = std::uniform_int_distribution<int>(1, 2)(random);

  clearPreview(true);
  fields_[Ciphertext] = enigma::makeMachine(key).processString(std::string(message));
  fields_[Crib] = message.substr(0, 53);
  fields_[SearchRings] = formatRings(key);
  fields_[SearchLeads] = formatLeads(key.plugPairs);
  fields_[Offset] = "1";
  rotorScope_ = RotorScope::FirstThree;
  reflectorScope_ = ReflectorScope::Both;
  boardMode_ = PlugboardMode::Known;
  focus_ = Crib;
  replace_ = false;
  status_ = "Challenge ready. Run or step to find its key.";
}

void DecipherPanel::clearPreview(bool both) {
  status_.clear();
  if (!searchMode_ || both) {
    knownPlaintext_.clear();
  }
  if (searchMode_ || both) {
    candidates_.clear();
    progress_ = {};
    selected_ = 0;
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
        if (value != fields_[focus_]) {
          clearPreview(focus_ == Ciphertext);
          fields_[focus_] = std::move(value);
        }
        replace_ = false;
        status_ = message ? "The message accepts up to 240 letters"
                          : "This field is full";
        return;
      }
      value.push_back(accepted);
    }
  }
  if (!value.empty() || !replace_) {
    if (value != fields_[focus_]) {
      clearPreview(focus_ == Ciphertext);
      fields_[focus_] = std::move(value);
    }
    replace_ = false;
  }
}

void DecipherPanel::run(bool step) {
  if (running_) {
    return;
  }
  clearPreview();
  try {
    if (!searchMode_) {
      auto settings =
          parseSettings(fields_[Start], fields_[Rings], fields_[Leads]);
      settings.rotorTypes = knownRotors_;
      settings.reflectorIndex = knownReflector_;
      knownPlaintext_ = decipher(fields_[Ciphertext], settings);
      status_ = "Plaintext ready";
      return;
    }

    auto request = parseSearch(
        fields_[Ciphertext], fields_[Crib], fields_[SearchRings],
        fields_[Offset], boardMode_,
        boardMode_ == PlugboardMode::Unknown ? "" : fields_[SearchLeads]);
    request.rotors = rotorScope_;
    request.reflectors = reflectorScope_;
    cribLength_ = fields_[Crib].size();
    pausedAtCheckpoint_ = false;
    {
      std::scoped_lock lock(mailboxMutex_);
      paused_ = step;
      stepRequested_ = step;
      waiting_ = false;
    }
    status_ = step ? "Stepping to the next checkpoint" : "Preparing search";
    worker_ = std::jthread([this, request = std::move(request)](
                               const std::stop_token &stop) {
      try {
        auto result = search(
            request, {.work = expandedSearchWork}, stop,
            [this, &stop](const SearchProgress &progress) {
              std::unique_lock lock(mailboxMutex_);
              pendingProgress_ = progress;
              if (progress.phase != SearchPhase::Finished) {
                waiting_ = paused_ && !stepRequested_;
                searchGate_.wait(lock, stop, [this] {
                  return !paused_ || stepRequested_;
                });
                waiting_ = false;
                stepRequested_ = false;
              }
            },
            [this](const Candidate &candidate) {
              std::scoped_lock lock(mailboxMutex_);
              pendingCandidates_.push_back(candidate);
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

void DecipherPanel::togglePause() {
  std::scoped_lock lock(mailboxMutex_);
  paused_ = !paused_;
  pausedAtCheckpoint_ = false;
  stepRequested_ = false;
  status_ = paused_ ? "Pausing at the next checkpoint" : "Resuming search";
  searchGate_.notify_all();
}

void DecipherPanel::stepSearch() {
  if (!running_) {
    run(true);
    return;
  }
  std::scoped_lock lock(mailboxMutex_);
  if (paused_ && waiting_) {
    pausedAtCheckpoint_ = false;
    stepRequested_ = true;
    status_ = "Stepping to the next checkpoint";
    searchGate_.notify_all();
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
    if (!worker_.get_stop_token().stop_requested()) {
      if (waiting_ && !stepRequested_) {
        pausedAtCheckpoint_ = true;
        status_ = "Paused at checkpoint";
      } else if (!paused_) {
        status_ = progress_.phase == SearchPhase::Preparing
                      ? "Preparing scramblers"
                      : "Checking settings against crib";
      }
    }
    changed = true;
  }
  if (!pendingCandidates_.empty()) {
    candidates_.insert(candidates_.end(),
                       std::make_move_iterator(pendingCandidates_.begin()),
                       std::make_move_iterator(pendingCandidates_.end()));
    pendingCandidates_.clear();
    changed = true;
  }
  if (pendingResult_) {
    auto result = std::move(*pendingResult_);
    pendingResult_.reset();
    progress_ = result.progress;
    candidates_ = std::move(result.candidates);
    running_ = false;
    pausedAtCheckpoint_ = false;
    if (result.status == SearchStatus::Complete && candidates_.empty()) {
      status_ = progress_.viableAlignments == 0 ? "No viable crib positions"
                                                : "No match in this search";
    } else {
      status_ = statusText(result.status);
    }
    changed = true;
  } else if (!pendingError_.empty()) {
    running_ = false;
    pausedAtCheckpoint_ = false;
    status_ = std::exchange(pendingError_, {});
    changed = true;
  }
  return changed && open_;
}

std::string_view DecipherPanel::plaintext() const {
  if (!searchMode_) {
    return knownPlaintext_;
  }
  return !candidates_.empty()
             ? std::string_view(candidates_[selected_].plaintext)
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
  const bool control = (static_cast<unsigned>(event.mods) &
                        static_cast<unsigned>(GLFW_MOD_CONTROL)) != 0;
  if ((click && hit(layout.back())) ||
      (press && event.key == GLFW_KEY_ESCAPE)) {
    worker_.request_stop();
    pausedAtCheckpoint_ = false;
    open_ = false;
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
  if (searchMode_) {
    if (running_ && click && hit(layout.cancel())) {
      worker_.request_stop();
      pausedAtCheckpoint_ = false;
      status_ = "Cancelling search";
      return true;
    }
    if (running_ && ((click && hit(layout.pause())) ||
                     (press && event.key == GLFW_KEY_F6))) {
      togglePause();
      return true;
    }
    if ((click && hit(layout.step())) || (press && event.key == GLFW_KEY_F7)) {
      stepSearch();
      return true;
    }
    if (click && !candidates_.empty() && hit(layout.loadKey())) {
      try {
        session.applySettings(candidates_[selected_].settings);
        status_ = "Preview key loaded in Workbench";
      } catch (const std::exception &error) {
        status_ = error.what();
      }
      return true;
    }
    if (click && hit(layout.previous()) && selected_ > 0) {
      --selected_;
      return true;
    }
    if (click && hit(layout.next()) && selected_ + 1 < candidates_.size()) {
      ++selected_;
      return true;
    }
  }
  if (running_) {
    return event.type == EventType::CursorPos;
  }
  if (press && control && event.key == GLFW_KEY_V) {
    edit(platform.getClipboard());
    return true;
  }
  if (press && control && event.key == GLFW_KEY_A) {
    replace_ = true;
    return true;
  }
  if (key && event.key == GLFW_KEY_BACKSPACE) {
    if (!fields_[focus_].empty()) {
      clearPreview(focus_ == Ciphertext);
      if (replace_) {
        fields_[focus_].clear();
      } else {
        fields_[focus_].pop_back();
      }
    }
    replace_ = false;
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

  if (hit(layout.knownTab()) || hit(layout.searchTab())) {
    searchMode_ = hit(layout.searchTab());
    focus_ = Ciphertext;
    replace_ = false;
    status_.clear();
  } else if (searchMode_ && hit(layout.challenge())) {
    try {
      loadChallenge();
    } catch (const std::exception &error) {
      status_ = error.what();
    }
  } else if (hit(layout.useSettings())) {
    copySettings(session.settings());
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
      for (std::size_t i = 0; i < rotorScopes.size(); ++i) {
        if (hit(layout.scope(static_cast<int>(i))) && rotorScope_ != rotorScopes[i]) {
          rotorScope_ = rotorScopes[i];
          clearPreview();
        }
      }
      if (hit(layout.scope(3))) {
        const auto next = (static_cast<std::size_t>(reflectorScope_) + 1) %
                          reflectorLabels.size();
        reflectorScope_ = static_cast<ReflectorScope>(next);
        clearPreview();
      }
      for (std::size_t i = 0; i < boardModes.size(); ++i) {
        if (hit(layout.boardMode(static_cast<int>(i))) && boardMode_ != boardModes[i]) {
          boardMode_ = boardModes[i];
          clearPreview();
          if (boardMode_ == PlugboardMode::Unknown && focus_ == SearchLeads) {
            focus_ = Crib;
          }
        }
      }
      if (hit(layout.leads(true))) {
        if (boardMode_ == PlugboardMode::Unknown) {
          boardMode_ = PlugboardMode::Partial;
          clearPreview();
        }
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
  const float titleY = assets.title.centeredLineTop(center(layout.back()).y - 3,
                                                    type::title);
  draw.addText(assets.title, "Decipher", {layout.body.x, titleY}, type::title);
  button(draw, ui, layout.back(), "Back to Workbench", cursor);
  draw.addLine({layout.body.x, 75}, {right(layout.body), 75}, 1, border);
  button(draw, ui, layout.knownTab(), "Known settings", cursor, !running_,
         selection(!searchMode_));
  button(draw, ui, layout.searchTab(), "Bombe search", cursor, !running_,
         selection(searchMode_));
  if (searchMode_) {
    button(draw, ui, layout.challenge(), "Try challenge", cursor, !running_);
  }
  draw.addBox(layout.form, panel);
  draw.addBox(layout.results, panel);
  button(draw, ui, layout.useSettings(), searchMode_ ? "Copy rings & leads" :
                                                       "Use Workbench key", cursor,
         !running_);

  const Candidate *match = searchMode_ && !candidates_.empty()
                               ? &candidates_[selected_] : nullptr;

  const auto field = [&](Field id, Rectangle bounds, std::string_view label,
                         bool enabled = true,
                         std::string_view placeholder = {},
                         bool showPlaceholder = false) {
    const bool focused = focus_ == id && enabled && !running_;
    draw.addText(ui, label, {bounds.x, bounds.y - 22}, type::control,
                 enabled ? foreground : muted);
    draw.addBox(bounds, focused && replace_ ? amberWell : well,
                focused ? amber : border, 0.15F);
    if (id == Ciphertext || id == Crib) {
      if (fields_[id].empty()) {
        draw.addText(ui, placeholder, {bounds.x + 10, bounds.y + 10},
                     type::control, muted);
      } else {
        const bool highlight = id == Ciphertext && match;
        tape(draw, mono, bounds, fields_[id], highlight ? match->cribOffset : 0,
             highlight ? cribLength_ : 0, true);
      }
    } else if (enabled) {
      const float textTop = mono.centeredLineTop(center(bounds).y, type::body);
      const bool empty = showPlaceholder || fields_[id].empty();
      draw.setScissor(insetBy(bounds, 5));
      draw.addText(mono, empty ? placeholder : std::string_view(fields_[id]),
                   {bounds.x + 10, textTop}, type::body,
                   empty ? muted : foreground);
      draw.resetScissor();
    }
  };
  field(Ciphertext, layout.ciphertext(), "Ciphertext", true, "Type or paste ciphertext");
  if (searchMode_) {
    field(Crib, layout.crib(), "Crib (expected plaintext)");
    field(SearchRings, layout.rings(true), "Rings (01-26, left to right)");
    field(Offset, layout.offset(), "Crib position", true, "Auto");
    for (std::size_t i = 0; i < rotorScopes.size(); ++i) {
      button(draw, ui, layout.scope(static_cast<int>(i)), rotorLabels[i], cursor,
             !running_, selection(rotorScope_ == rotorScopes[i]));
    }
    button(draw, ui, layout.scope(3),
           reflectorLabels[static_cast<std::size_t>(reflectorScope_)], cursor,
           !running_);
    for (std::size_t i = 0; i < boardModes.size(); ++i) {
      button(draw, ui, layout.boardMode(static_cast<int>(i)), boardLabels[i],
             cursor, !running_, selection(boardMode_ == boardModes[i]));
    }
    field(SearchLeads, layout.leads(true), "Plugboard pairs (AZ BY)", true,
          boardMode_ == PlugboardMode::Unknown ? "Click to enter known pairs" : "",
          boardMode_ == PlugboardMode::Unknown);
  } else {
    field(Start, layout.start(), "Starting windows (3-5 letters)");
    field(Rings, layout.rings(false), "Rings (01-26, left to right)");
    field(Leads, layout.leads(false), "Plugboard pairs (AZ BY)");
    enigma::MachineSettings key;
    key.rotorCount = std::min(knownRotors_.size(), fields_[Start].size());
    key.rotorTypes = knownRotors_;
    key.reflectorIndex = knownReflector_;
    draw.addText(ui, formatProfile(key),
                 {layout.form.x + 16, bottom(layout.leads(false)) + 14},
                 type::control, muted);
  }
  button(draw, ui, layout.run(), searchMode_ ? "Run search" : "Decipher",
         cursor, !running_, ButtonTone::Primary);
  if (searchMode_) {
    button(draw, ui, layout.pause(), paused_ && running_ ? "Resume" : "Pause",
           cursor, running_);
    button(draw, ui, layout.step(), "Step", cursor, !running_ || isPaused());
    button(draw, ui, layout.cancel(), "Cancel", cursor, running_);
  }
  const float resultX = layout.results.x + 16;
  draw.addText(ui, searchMode_ ? "Bombe results" : "Plaintext", {resultX, 156},
               type::section);
  draw.addText(ui, status_, {resultX, 191}, type::body, amber);
  if (searchMode_) {
    const float drumsRight = right(layout.results) - 20;
    const float drumsWidth = 230;
    const float drumsX = drumsRight - drumsWidth;
    const float drumsY = 215;
    const float drumsHeight = 77;

    draw.addText(mono, std::to_string(progress_.viableAlignments) + "/" +
                           std::to_string(progress_.alignments) + " viable positions",
                 {resultX, 222}, type::caption, muted);
    draw.addText(mono, std::to_string(progress_.settingsChecked) + "/" +
                           std::to_string(progress_.totalSettings) + " checked",
                 {resultX, 241}, type::caption, muted);
    draw.addText(mono, std::to_string(candidates_.size()) +
                           (candidates_.size() == 1 ? " match" : " matches"),
                 {resultX, 260}, type::caption, candidates_.empty() ? muted : amber);
    const Rectangle meter{resultX, 283, drumsX - resultX - 14, 4};
    draw.addRectangle(meter, border);
    if (progress_.totalSettings > 0) {
      auto fill = meter;
      fill.width *= static_cast<float>(progress_.settingsChecked) /
                    static_cast<float>(progress_.totalSettings);
      draw.addRectangle(fill, amber);
    }

    draw.addBox({drumsX, drumsY, drumsWidth, drumsHeight}, well, border, 0.12F);
    std::array<int, rotorCount> models = progress_.rotors;
    std::array<int, rotorCount> starts{};
    auto reflector = progress_.reflector;
    bool hasWindows = progress_.start.has_value();
    if (match && !running_) {
      hasWindows = true;
      std::copy_n(match->settings.rotorTypes.begin(), rotorCount, models.begin());
      std::copy_n(match->settings.starts.begin(), rotorCount, starts.begin());
      reflector = match->settings.reflectorIndex;
    } else if (progress_.start) {
      auto encoded = *progress_.start;
      for (auto i = starts.size(); i > 0; --i) {
        starts[i - 1] = static_cast<int>(encoded % 26);
        encoded /= 26;
      }
    }
    std::string drumTitle = "Scramblers";
    if (match && !running_) {
      drumTitle = std::string("Match key  ·  ") + enigma::reflectorNames[reflector];
    } else if (progress_.profile > 0) {
      drumTitle = std::to_string(progress_.profile) + "/" +
                  std::to_string(progress_.profiles) + "  ·  " +
                  enigma::reflectorNames[reflector];
      if (progress_.cribOffset) {
        drumTitle += "  ·  pos " + std::to_string(*progress_.cribOffset + 1);
      }
    }
    draw.addCenteredText(ui, drumTitle,
                         {drumsX + drumsWidth * 0.5F, drumsY + 12},
                         type::caption, muted);
    const std::array<Color, 3> drumColors{
        Color{60, 20, 25, 255},  // Red drum
        Color{18, 32, 60, 255},  // Blue drum
        Color{18, 50, 30, 255}   // Green drum
    };

    for (int d = 0; d < 3; ++d) {
      const float dcx = drumsX + 38.0F + static_cast<float>(d) * 77.0F;
      const float dcy = drumsY + 39.0F;
      const float drumRadius = 17.0F;

      draw.addCircle({dcx, dcy}, drumRadius + 2.0F, Color{175, 145, 65, 255});
      draw.addCircle({dcx, dcy}, drumRadius + 0.5F, Color{85, 70, 30, 255});
      draw.addCircle({dcx, dcy}, drumRadius - 1.0F, drumColors[d]);

      if (hasWindows) {
        const auto pos = starts[d];
        draw.addCenteredGlyph(mono, indexToChar((pos + 25) % 26), {dcx, dcy - 10}, 9, muted);
        draw.addCenteredGlyph(mono, indexToChar(pos), {dcx, dcy}, 14, amber);
        draw.addCenteredGlyph(mono, indexToChar((pos + 1) % 26), {dcx, dcy + 10}, 9, muted);
      } else {
        draw.addCenteredText(mono, "–", {dcx, dcy}, 14, muted);
      }
      draw.addCenteredText(ui, enigma::rotorNames[models[d]], {dcx, drumsY + 66},
                           type::caption, muted);
    }
  }
  std::size_t highlight = 0;
  std::size_t highlightedLength = 0;
  if (match) {
    highlight = match->cribOffset;
    highlightedLength = cribLength_;
    button(draw, ui, layout.previous(), "Previous", cursor, selected_ > 0);
    button(draw, ui, layout.next(), "Next", cursor,
           selected_ + 1 < candidates_.size());
    const auto matchLabelX = (right(layout.previous()) + layout.next().x) * 0.5F;
    const auto matchLabel = "Crib match " + std::to_string(selected_ + 1) + " / " +
                            std::to_string(candidates_.size());
    draw.addCenteredText(ui, matchLabel,
                         {matchLabelX, center(layout.previous()).y}, type::control);
    draw.addText(mono, formatProfile(match->settings), {resultX, 340}, type::body);
    draw.addText(mono, "Start " + formatStarts(match->settings) + "  /  Rings " +
                           formatRings(match->settings),
                 {resultX, 362}, type::control);
    const auto leads = formatLeads(match->settings.plugPairs);
    draw.addText(mono, "Leads " + (leads.empty() ? std::string("none") : leads),
                 {resultX, 383}, type::control);
    draw.addText(mono, "Crib position " + std::to_string(match->cribOffset + 1),
                 {resultX, 405}, type::control, muted);
    if (!match->unresolvedSockets.empty()) {
      draw.addText(mono, "Assumed unplugged " + match->unresolvedSockets,
                   {resultX, 425}, type::caption, muted);
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
  if (match) {
    button(draw, ui, layout.loadKey(), "Load key", cursor);
  }
  draw.addText(ui, searchMode_ ? "Ctrl+V  Paste    ·    F6  Pause / resume    ·    F7  Step    ·    Esc  Back"
                              : "Ctrl+V  Paste    ·    Ctrl+A  Select field    ·    Esc  Back",
               {layout.body.x, ui.centeredLineTop(static_cast<float>(height) - 22,
                                                 type::control)},
               type::control, muted);
}

} // namespace bombe
