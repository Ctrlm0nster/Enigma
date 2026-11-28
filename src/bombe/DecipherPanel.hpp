#pragma once

#include "bombe/Bombe.hpp"
#include "workbench/Geometry.hpp"

#include <array>
#include <condition_variable>
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
  bool isPaused() const { return pausedAtCheckpoint_; }
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
  void loadChallenge();
  void createChallenge(const workbench::Session &session);
  void loadSearch(SearchRequest request);
  SearchRequest searchInputs() const;
  void previewChallenge();
  void clearPreview(bool both = false);
  void clearForEdit();
  void edit(std::string_view text);
  void run(bool step = false);
  void togglePause();
  void stepSearch();
  std::string_view plaintext() const;

  bool open_ = false;
  bool searchMode_ = false;
  bool running_ = false;
  bool pausedAtCheckpoint_ = false;
  bool replace_ = false;
  Field focus_ = Ciphertext;
  PlugboardMode boardMode_ = PlugboardMode::Unknown;
  RotorScope rotorScope_ = RotorScope::FirstThree;
  ReflectorScope reflectorScope_ = ReflectorScope::B;
  std::array<std::string, FieldCount> fields_{
      {"", "", "AAA", "01 01 01", "01 01 01", "", "", ""}};
  std::array<int, enigma::maximumRotorCount> knownRotors_ =
      enigma::MachineSettings{}.rotorTypes;
  int knownReflector_ = enigma::MachineSettings{}.reflectorIndex;
  std::string knownPlaintext_;
  std::string status_;
  std::string searchOutcome_;
  std::array<std::string, 2> submittedSetup_;
  struct ChallengeSource {
    std::string plaintext;
    enigma::MachineSettings key;
  };
  std::optional<ChallengeSource> challengeSource_;
  std::string sharePreview_;
  std::string shareStatus_;
  SearchProgress progress_;
  std::vector<Candidate> candidates_;
  std::size_t selected_ = 0;
  std::size_t cribLength_ = 0;
  std::mutex mailboxMutex_;
  std::condition_variable_any searchGate_;
  bool paused_ = false;
  bool stepRequested_ = false;
  bool waiting_ = false;
  std::optional<SearchProgress> pendingProgress_;
  std::vector<Candidate> pendingCandidates_;
  std::optional<SearchResult> pendingResult_;
  std::string pendingError_;
  std::jthread worker_;
};

} // namespace bombe
