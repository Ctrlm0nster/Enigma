#include "Charactermap.hpp"
#include "bombe/Bombe.hpp"
#include "bombe/DecipherInput.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <stdexcept>

namespace {

constexpr std::string_view message =
    "THISPREFACESETSUPTHEMESSAGEFORANENIGMADEMONSTRATION"
    "BRINGEXTRAQUIETSUPPLIESFORTHEWEATHERSTATIONNEARZURICH";

enigma::MachineSettings demonstrationSettings() {
  return bombe::parseSettings("ADV", "03 02 05", "AZ BY CR");
}

bombe::SearchRequest demonstrationSearch(bombe::PlugboardMode mode) {
  const auto settings = demonstrationSettings();
  auto sender = enigma::makeMachine(settings);
  auto request =
      bombe::parseSearch(sender.processString(std::string(message)),
                         std::string(message.substr(1, 85)),
                         "03 02 05", "2", mode, "");
  return request;
}

} // namespace

TEST_CASE("Known settings decipher the documented Enigma vector", "[bombe]") {
  CHECK(bombe::decipher(
            "bdz go", bombe::parseSettings("AAA", "01,01,01", "")) == "AAAAA");

  const auto settings =
      bombe::parseSettings("ABBCE", "01 03 09 24 26", "AZ BY");
  const auto ciphertext =
      enigma::makeMachine(settings).processString(std::string(message));
  CHECK(bombe::decipher(ciphertext, settings) == message);
}

TEST_CASE("Bombe search finds the pre-message start across a double step",
          "[bombe]") {
  auto settings = demonstrationSettings();
  auto machine = enigma::makeMachine(settings);
  auto windowsEqual = [&](std::string_view expected) {
    for (std::size_t i = 0; i < expected.size(); ++i) {
      CHECK(indexToChar(machine.getRotors()[i].getPosition()) == expected[i]);
    }
  };
  machine.stepRotors();
  windowsEqual("AEW");
  const auto map = machine.scrambler();
  for (std::size_t i = 0; i < map.size(); ++i) {
    CHECK(map[map[i]] == i);
    CHECK(map[i] != i);
  }
  machine.stepRotors();
  windowsEqual("BFX");

  auto request = demonstrationSearch(bombe::PlugboardMode::Known);
  request.knownLeads = settings.plugPairs;
  const auto result = bombe::search(request);
  REQUIRE(result.status == bombe::SearchStatus::Complete);
  CHECK(result.progress.settingsChecked == bombe::startCount);
  // ADV and AEV converge on BFX at the second letter, where the crib starts.
  REQUIRE(result.candidates.size() == 2);
  const auto &match = result.candidates.front();
  CHECK(match.settings.starts == settings.starts);
  CHECK(match.cribOffset == 1);
  CHECK(match.unresolvedSockets.empty());
  CHECK(match.plaintext == message);
  const auto &alternative = result.candidates.back();
  CHECK(bombe::formatStarts(alternative.settings) == "AEV");
  CHECK(alternative.plaintext.front() != message.front());
  CHECK(alternative.plaintext.substr(1) == message.substr(1));
}

TEST_CASE("Bombe infers unknown leads and verifies the full tape", "[bombe]") {
  auto request = demonstrationSearch(bombe::PlugboardMode::Unknown);
  SECTION("Known leads") {
    request.plugboard = bombe::PlugboardMode::Partial;
    request.knownLeads = {{charToIndex('A'), charToIndex('Z')}};
  }
  const auto result = bombe::search(request);
  REQUIRE(result.status == bombe::SearchStatus::Complete);
  const auto match =
      std::ranges::find_if(result.candidates, [](const auto &candidate) {
        return bombe::formatStarts(candidate.settings) == "ADV" &&
               bombe::formatLeads(candidate.settings.plugPairs) == "AZ BY CR" &&
               candidate.plaintext == message;
      });
  REQUIRE(match != result.candidates.end());
  for (const auto &candidate : result.candidates) {
    CHECK(candidate.plaintext.substr(candidate.cribOffset,
                                     request.crib.size()) == request.crib);
  }
}

TEST_CASE("Bombe recovers rotor order and reflector and streams verified matches",
          "[bombe]") {
  auto settings = demonstrationSettings();
  auto scope = bombe::RotorScope::Fixed;
  auto reflectors = bombe::ReflectorScope::B;
  std::size_t profiles = 0;
  bool stopAtMatch = false;

  SECTION("Three rotor orders with either reflector") {
    settings.rotorTypes = {2, 0, 1, 3, 4};
    settings.reflectorIndex = 2;
    scope = bombe::RotorScope::FirstThree;
    reflectors = bombe::ReflectorScope::Both;
    profiles = 12;
  }
  SECTION("Orders drawn from five rotors") {
    settings.rotorTypes = {0, 1, 3, 3, 4};
    settings.reflectorIndex = 1;
    scope = bombe::RotorScope::FirstFive;
    reflectors = bombe::ReflectorScope::B;
    profiles = 60;
    stopAtMatch = true;
  }

  bombe::SearchRequest request;
  request.ciphertext =
      enigma::makeMachine(settings).processString(std::string(message));
  request.crib = message;
  request.cribOffset = 0;
  std::copy_n(settings.rings.begin(), bombe::rotorCount, request.rings.begin());
  request.plugboard = bombe::PlugboardMode::Known;
  request.knownLeads = settings.plugPairs;
  request.rotors = scope;
  request.reflectors = reflectors;

  std::stop_source stop;
  bool streamed = false;
  std::optional<bombe::SearchProgress> checkpoint;
  const auto hasExpectedKey = [&](const auto &candidate) {
    return candidate.settings.rotorTypes == settings.rotorTypes &&
           candidate.settings.reflectorIndex == settings.reflectorIndex &&
           candidate.settings.starts == settings.starts;
  };
  const auto result = bombe::search(
      request, {}, stop.get_token(),
      [&](const auto &progress) {
        if (progress.phase == bombe::SearchPhase::Checking && !checkpoint) {
          checkpoint = progress;
        }
      },
      [&](const auto &candidate) {
        CHECK(candidate.plaintext == message);
        if (hasExpectedKey(candidate)) {
          streamed = true;
          if (stopAtMatch) {
            stop.request_stop();
          }
        }
      });
  if (!checkpoint || !checkpoint->start) {
    FAIL("Bombe did not publish a starting window at a checkpoint");
    return;
  }
  CHECK(checkpoint->work == bombe::checkpointWork);
  CHECK(*checkpoint->start == bombe::checkpointWork - 1);
  CHECK(checkpoint->cribOffset == 0);
  CHECK(streamed);
  CHECK(std::ranges::any_of(result.candidates, hasExpectedKey));
  CHECK(result.progress.profiles == profiles);
  CHECK(result.progress.totalSettings == profiles * bombe::startCount);
  CHECK(result.progress.phase == bombe::SearchPhase::Finished);
  if (stopAtMatch) {
    CHECK(result.status == bombe::SearchStatus::Cancelled);
  } else {
    CHECK(result.status == bombe::SearchStatus::Complete);
    CHECK(result.progress.settingsChecked == result.progress.totalSettings);
  }
}

TEST_CASE("Bombe aligns normalized text and rejects self-matches", "[bombe]") {
  auto request = demonstrationSearch(bombe::PlugboardMode::Known);
  request.knownLeads = demonstrationSettings().plugPairs;
  request.cribOffset.reset();
  const auto result = bombe::search(request);
  CHECK(result.progress.alignments ==
        request.ciphertext.size() - request.crib.size() + 1);
  REQUIRE(result.candidates.size() == 2);
  CHECK(std::ranges::all_of(result.candidates, [](const auto &candidate) {
    return candidate.cribOffset == 1;
  }));

  const auto impossible = bombe::parseSearch("a a-a!", "Aa", "01 01 01", "",
                                             bombe::PlugboardMode::Unknown, "");
  const auto exhausted = bombe::search(impossible);
  CHECK(exhausted.status == bombe::SearchStatus::Complete);
  CHECK(exhausted.progress.viableAlignments == 0);
  CHECK(exhausted.candidates.empty());
}

TEST_CASE("Disconnected Bombe menu agrees with an exhaustive plugboard check",
          "[bombe]") {
  constexpr std::string_view leads = "AB CD EF GH IJ KL MN OP QR";
  auto settings = bombe::parseSettings("AAA", "01 01 01", leads);
  const auto request = bombe::parseSearch("UH", "SW", "01 01 01", "1",
                                          bombe::PlugboardMode::Partial, leads);
  std::set<std::string> oracle;
  const auto runReference = [&](const auto &key, std::string visibleLeads) {
    if (enigma::makeMachine(key).processString(request.ciphertext) ==
        request.crib) {
      oracle.insert(std::move(visibleLeads));
    }
  };
  runReference(settings, bombe::formatLeads(settings.plugPairs));
  // Nine known leads leave room for one pair among the eight free sockets.
  for (char a = 'S'; a <= 'Z'; ++a) {
    for (char b = static_cast<char>(a + 1); b <= 'Z'; ++b) {
      auto complete = settings;
      complete.plugPairs.emplace_back(charToIndex(a), charToIndex(b));
      constexpr std::string_view menuLetters = "SUWH";
      const bool touchesMenu = menuLetters.find(a) != std::string_view::npos ||
                               menuLetters.find(b) != std::string_view::npos;
      const auto &visible = touchesMenu ? complete : settings;
      runReference(complete, bombe::formatLeads(visible.plugPairs));
    }
  }
  REQUIRE_FALSE(oracle.empty());

  // Two disconnected edges need at most two partner guesses each for start AAA.
  const auto result = bombe::search(request, {.work = 2'000, .results = 300});
  std::set<std::string> found;
  for (const auto &candidate : result.candidates) {
    if (bombe::formatStarts(candidate.settings) == "AAA") {
      found.insert(bombe::formatLeads(candidate.settings.plugPairs));
    }
  }
  CHECK(found == oracle);
}

TEST_CASE("Bombe cancellation and limits preserve honest result states",
          "[bombe]") {
  const auto request = bombe::parseSearch("B", "A", "01 01 01", "1",
                                          bombe::PlugboardMode::Unknown, "");
  const auto limited = bombe::search(request, {.work = 300, .results = 2});
  CHECK(limited.status == bombe::SearchStatus::ResultLimit);
  REQUIRE(limited.candidates.size() == 2);
  for (const auto &candidate : limited.candidates) {
    CHECK_FALSE(candidate.unresolvedSockets.empty());
    CHECK(candidate.plaintext == "A");
  }

  const auto budget = bombe::search(request, {.work = 1, .results = 2});
  CHECK(budget.status == bombe::SearchStatus::WorkLimit);
  CHECK(budget.candidates.empty());

  std::stop_source stop;
  auto longer = demonstrationSearch(bombe::PlugboardMode::Unknown);
  const auto cancelled =
      bombe::search(longer, {}, stop.get_token(), [&](const auto &progress) {
        if (progress.work >= 256) {
          stop.request_stop();
        }
      });
  CHECK(cancelled.status == bombe::SearchStatus::Cancelled);
  CHECK(cancelled.progress.work == 256);
}

TEST_CASE("Bombe rejects invalid input and conflicting leads", "[bombe]") {
  CHECK_THROWS_AS(bombe::decipher(std::string(241, 'A'), {}),
                  std::invalid_argument);
  CHECK_THROWS_AS(bombe::decipher("...", {}), std::invalid_argument);
  CHECK_THROWS_AS(bombe::parseSettings("AAA", "01 00 01", ""),
                  std::invalid_argument);
  CHECK_THROWS_AS(bombe::parseSettings("AAA", "01 01", ""),
                  std::invalid_argument);

  auto request = bombe::parseSearch("BC", "A", "01 01 01", "3",
                                    bombe::PlugboardMode::Partial, "AZ AB");
  CHECK_THROWS_AS(bombe::search(request), std::invalid_argument);
  request.cribOffset = 0;
  CHECK_THROWS_AS(bombe::search(request), std::invalid_argument);
  request.knownLeads.clear();
  request.crib = "AAAA";
  CHECK_THROWS_AS(bombe::search(request), std::invalid_argument);
}
