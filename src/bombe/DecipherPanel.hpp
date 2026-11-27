#pragma once

#include "bombe/Bombe.hpp"
#include "workbench/Geometry.hpp"

#include <array>
#include <chrono>
#include <mutex>
#include <thread>

namespace workbench {
struct Assets;
class DrawList;
struct InputEvent;
struct PlatformContext;
class Session;
} // namespace workbench

namespace bombe {

class DecipherPanel {
public:
  bool isOpen() const { return open_; }
  bool isRunning() const { return running_; }
  bool update();

  // An empty result lets the Workbench handle the event. A value reports
  // redraw.
  std::optional<bool> handle(const workbench::InputEvent &event,
                             const workbench::PlatformContext &platform,
                             workbench::Session &session, int width,
                             int height);
  void draw(workbench::DrawList &draw, const workbench::Assets &assets,
            workbench::Vector2 cursor, int width, int height) const;

private:
  enum Field {
    Ciphertext,
    Crib,
    Start,
    Rings,
    SearchRings,
    Offset,
    Leads,
    SearchLeads,
    FieldCount
  };

  void copySettings(enigma::MachineSettings settings);
  void edit(std::string_view text);
  void run();
  std::string_view plaintext() const;

  bool open_ = false;
  bool searchMode_ = false;
  bool running_ = false;
  bool replace_ = false;
  Field focus_ = Ciphertext;
  PlugboardMode boardMode_ = PlugboardMode::Unknown;
  std::array<std::string, FieldCount> fields_{
      {"", "", "AAA", "01 01 01", "01 01 01", "", "", ""}};
  std::string knownPlaintext_;
  std::string status_;
  SearchProgress progress_;
  std::optional<SearchResult> result_;
  std::size_t selected_ = 0;
  std::size_t cribLength_ = 0;
  std::chrono::steady_clock::time_point startTime_{};
  double searchDurationSeconds_ = 0.0;

  std::mutex mailboxMutex_;
  std::optional<SearchProgress> pendingProgress_;
  std::optional<SearchResult> pendingResult_;
  std::string pendingError_;
  std::jthread worker_;
};

} // namespace bombe
