#include "bombe/Bombe.hpp"

#include "Charactermap.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace bombe {
namespace {

constexpr int alphabetSize = 26;
constexpr int unassigned = -1;

std::string normalize(std::string_view text, std::string_view name) {
  std::string result;
  result.reserve(std::min(text.size(), enigma::messageLimit));
  for (char character : text) {
    if (const char letter = enigma::normalizeLetter(character); letter != 0) {
      if (result.size() == enigma::messageLimit) {
        throw std::invalid_argument(std::string(name) + " exceeds 240 letters");
      }
      result.push_back(letter);
    }
  }
  if (result.empty()) {
    throw std::invalid_argument(std::string(name) +
                                " needs at least one A–Z letter");
  }
  return result;
}

struct Edge {
  int plain;
  int cipher;
  std::size_t position;
};

struct Menu {
  std::size_t offset = 0;
  std::vector<Edge> edges;
  std::array<std::vector<std::size_t>, alphabetSize> incident;
  std::array<int, alphabetSize> vertices;
};

std::optional<Menu> makeMenu(std::string_view ciphertext, std::string_view crib,
                             std::size_t offset) {
  Menu menu;
  menu.offset = offset;
  menu.edges.reserve(crib.size());
  for (std::size_t i = 0; i < crib.size(); ++i) {
    const int plain = charToIndex(crib[i]);
    const int cipher = charToIndex(ciphertext[offset + i]);
    if (plain == cipher) {
      return std::nullopt;
    }
    menu.incident[idx(plain)].push_back(i);
    menu.incident[idx(cipher)].push_back(i);
    menu.edges.push_back({plain, cipher, offset + i});
  }
  std::iota(menu.vertices.begin(), menu.vertices.end(), 0);
  std::stable_sort(
      menu.vertices.begin(), menu.vertices.end(), [&](int a, int b) {
        return menu.incident[idx(a)].size() > menu.incident[idx(b)].size();
      });
  return menu;
}

struct Board {
  std::array<int, alphabetSize> partners;
  std::size_t pairCount = 0;

  Board() { partners.fill(unassigned); }

  int at(int letter) const { return partners[idx(letter)]; }

  bool assign(int letter, int partner) {
    if (at(letter) != unassigned || at(partner) != unassigned) {
      return at(letter) == partner && at(partner) == letter;
    }
    if (letter != partner) {
      if (pairCount == enigma::plugLimit) {
        return false;
      }
      ++pairCount;
    }
    partners[idx(letter)] = partner;
    partners[idx(partner)] = letter;
    return true;
  }
};

struct RotorState {
  EnigmaMachine::Permutation scrambler;
  std::uint16_t next;
};

std::uint16_t encodeWindows(const std::vector<Rotor> &rotors) {
  int encoded = 0;
  for (const auto &rotor : rotors) {
    encoded = encoded * alphabetSize + rotor.getPosition();
  }
  return static_cast<std::uint16_t>(encoded);
}

void decodeWindows(std::size_t encoded, enigma::MachineSettings &settings) {
  for (std::size_t i = rotorCount; i > 0; --i) {
    settings.starts[i - 1] = static_cast<int>(encoded % alphabetSize);
    encoded /= alphabetSize;
  }
}

struct Profile {
  std::array<int, rotorCount> rotors;
  int reflector;
};

std::vector<Profile> searchProfiles(const SearchRequest &request) {
  std::vector<Profile> profiles;
  const int firstReflector = request.reflectors == ReflectorScope::C ? 2 : 1;
  const int lastReflector = request.reflectors == ReflectorScope::B ? 1 : 2;
  for (int reflector = firstReflector; reflector <= lastReflector; ++reflector) {
    if (request.rotors == RotorScope::Fixed) {
      profiles.push_back({{0, 1, 2}, reflector});
      continue;
    }
    const int count = request.rotors == RotorScope::FirstFive ? 5 : 3;
    for (int left = 0; left < count; ++left) {
      for (int middle = 0; middle < count; ++middle) {
        for (int right = 0; right < count; ++right) {
          if (left != middle && left != right && middle != right) {
            profiles.push_back({{left, middle, right}, reflector});
          }
        }
      }
    }
  }
  return profiles;
}

using Scramblers =
    std::array<const EnigmaMachine::Permutation *, enigma::messageLimit>;

bool propagate(const Menu &menu, Board &board, const Scramblers &scramblers) {
  std::array<int, alphabetSize> pending{};
  std::size_t end = 0;
  for (int letter : menu.vertices) {
    if (board.at(letter) != unassigned) {
      pending[end++] = letter;
    }
  }
  for (std::size_t next = 0; next < end; ++next) {
    const int letter = pending[next];
    for (const auto edgeIndex : menu.incident[idx(letter)]) {
      const auto &edge = menu.edges[edgeIndex];
      const int other = edge.plain == letter ? edge.cipher : edge.plain;
      const int partner = (*scramblers[edge.position])[idx(board.at(letter))];
      const bool newAssignment = board.at(other) == unassigned;
      if (!board.assign(other, partner)) {
        return false;
      }
      if (newAssignment) {
        pending[end++] = other;
        if (partner != other) {
          pending[end++] = partner;
        }
      }
    }
  }
  return true;
}

class Search {
public:
  Search(const std::string &ciphertext, std::string_view crib,
         const enigma::MachineSettings &settings, SearchLimits limits,
         std::stop_token stop, const ProgressCallback &callback,
         const CandidateCallback &candidateCallback)
      : ciphertext_(ciphertext), crib_(crib), settings_(settings),
        limits_(limits), stop_(stop), callback_(callback),
        candidateCallback_(candidateCallback) {}

  SearchResult run(const std::vector<Menu> &menus, std::size_t alignmentCount,
                   const Board &seed, bool boardKnown,
                   const std::vector<Profile> &profiles) {
    result_.progress.alignments = alignmentCount;
    result_.progress.viableAlignments = menus.size();
    result_.progress.profiles = profiles.size();
    result_.progress.totalSettings = profiles.size() * menus.size() * startCount;
    if (!menus.empty()) {
      runProfiles(menus, seed, boardKnown, profiles);
    }
    interrupted();
    result_.progress.phase = SearchPhase::Finished;
    publish();
    return std::move(result_);
  }

private:
  void runProfiles(const std::vector<Menu> &menus, const Board &seed,
                   bool boardKnown, const std::vector<Profile> &profiles) {
    auto &progress = result_.progress;
    for (const auto &profile : profiles) {
      if (interrupted()) {
        return;
      }
      if (progress.work == limits_.work) {
        result_.status = SearchStatus::WorkLimit;
        return;
      }
      ++progress.profile;
      progress.rotors = profile.rotors;
      progress.reflector = profile.reflector;
      progress.phase = SearchPhase::Preparing;
      progress.start.reset();
      progress.cribOffset.reset();
      std::ranges::copy(profile.rotors, settings_.rotorTypes.begin());
      settings_.reflectorIndex = profile.reflector;
      publish();
      const auto states = rotorTable();
      if (!interrupted()) {
        progress.phase = SearchPhase::Checking;
        testStarts(menus, states, seed, boardKnown);
      }
    }
  }

  bool interrupted() {
    if (result_.status == SearchStatus::Complete && stop_.stop_requested()) {
      result_.status = SearchStatus::Cancelled;
    }
    return result_.status != SearchStatus::Complete;
  }

  void publish() const {
    if (callback_) {
      callback_(result_.progress);
    }
  }

  bool spendWork() {
    if (interrupted()) {
      return false;
    }
    if (result_.progress.work == limits_.work) {
      result_.status = SearchStatus::WorkLimit;
      return false;
    }
    if (result_.progress.work != 0 &&
        result_.progress.work % checkpointWork == 0) {
      publish();
      if (interrupted()) {
        return false;
      }
    }
    ++result_.progress.work;
    return true;
  }

  std::vector<RotorState> rotorTable() {
    std::vector<RotorState> states;
    states.reserve(startCount);
    auto settings = settings_;
    for (std::size_t i = 0; i < startCount && !interrupted(); ++i) {
      decodeWindows(i, settings);
      auto machine = enigma::makeMachine(settings);
      const auto scrambler = machine.scrambler();
      machine.stepRotors();
      states.push_back({scrambler, encodeWindows(machine.getRotors())});
    }
    return states;
  }

  void testStarts(const std::vector<Menu> &menus,
                  const std::vector<RotorState> &states, const Board &seed,
                  bool boardKnown) {
    const auto lastPosition = menus.back().edges.back().position;
    Scramblers scramblers{};
    for (std::size_t start = 0; start < startCount && !interrupted(); ++start) {
      std::size_t current = start;
      for (std::size_t position = 0; position <= lastPosition; ++position) {
        current = states[current].next;
        scramblers[position] = &states[current].scrambler;
      }
      for (const auto &menu : menus) {
        if (!spendWork()) {
          return;
        }
        result_.progress.start = start;
        result_.progress.cribOffset = menu.offset;
        ++result_.progress.settingsChecked;
        if (boardKnown) {
          const bool matches =
              std::ranges::all_of(menu.edges, [&](const auto &edge) {
                return seed.at(edge.cipher) ==
                       (*scramblers[edge.position])[idx(seed.at(edge.plain))];
              });
          if (matches) {
            save(start, menu.offset, seed);
          }
        } else {
          solve(start, menu, scramblers, seed);
        }
      }
    }
  }

  void solve(std::size_t start, const Menu &menu, const Scramblers &scramblers,
             Board board) {
    if (!propagate(menu, board, scramblers)) {
      return;
    }
    const auto next = std::ranges::find_if(menu.vertices, [&](int letter) {
      return !menu.incident[idx(letter)].empty() &&
             board.at(letter) == unassigned;
    });
    if (next == menu.vertices.end()) {
      save(start, menu.offset, board);
      return;
    }

    for (int partner = 0; partner < alphabetSize; ++partner) {
      if (!spendWork()) {
        return;
      }
      auto branch = board;
      if (branch.assign(*next, partner)) {
        solve(start, menu, scramblers, branch);
      }
    }
  }

  void save(std::size_t start, std::size_t offset, const Board &board) {
    Candidate candidate;
    candidate.settings = settings_;
    decodeWindows(start, candidate.settings);
    candidate.cribOffset = offset;
    for (int letter = 0; letter < alphabetSize; ++letter) {
      const int partner = board.at(letter);
      if (partner == unassigned) {
        candidate.unresolvedSockets.push_back(indexToChar(letter));
      } else if (letter < partner) {
        candidate.settings.plugPairs.emplace_back(letter, partner);
      }
    }
    auto replay = enigma::makeMachine(candidate.settings);
    candidate.plaintext = replay.processString(ciphertext_);
    if (candidate.plaintext.compare(offset, crib_.size(), crib_) != 0) {
      throw std::logic_error("Bombe stop disagrees with Enigma replay");
    }
    result_.candidates.push_back(std::move(candidate));
    result_.progress.matches = result_.candidates.size();
    if (candidateCallback_) {
      candidateCallback_(result_.candidates.back());
    }
    if (result_.candidates.size() == limits_.results) {
      result_.status = SearchStatus::ResultLimit;
    }
  }

  const std::string &ciphertext_;
  std::string_view crib_;
  enigma::MachineSettings settings_;
  SearchLimits limits_;
  std::stop_token stop_;
  const ProgressCallback &callback_;
  const CandidateCallback &candidateCallback_;
  SearchResult result_;
};

} // namespace

std::string decipher(std::string_view ciphertext,
                     const enigma::MachineSettings &settings) {
  return enigma::makeMachine(settings).processString(
      normalize(ciphertext, "Ciphertext"));
}

SearchResult search(const SearchRequest &request, SearchLimits limits,
                    std::stop_token stop, const ProgressCallback &progress,
                    const CandidateCallback &candidate) {
  if (limits.work == 0 || limits.results == 0) {
    throw std::invalid_argument("Search limits must be positive");
  }
  const auto ciphertext = normalize(request.ciphertext, "Ciphertext");
  const auto crib = normalize(request.crib, "Crib");
  if (crib.size() > ciphertext.size()) {
    throw std::invalid_argument("Crib is longer than the ciphertext");
  }
  const auto lastOffset = ciphertext.size() - crib.size();
  if (request.cribOffset && *request.cribOffset > lastOffset) {
    throw std::invalid_argument("Crib position is outside the ciphertext");
  }
  if (request.plugboard == PlugboardMode::Unknown &&
      !request.knownLeads.empty()) {
    throw std::invalid_argument(
        "Use a partial or known plugboard to supply leads");
  }

  enigma::MachineSettings settings;
  std::ranges::copy(request.rings, settings.rings.begin());
  settings.plugPairs = request.knownLeads;
  const auto seedMachine = enigma::makeMachine(settings);
  settings.plugPairs.clear();
  Board seed;
  for (const auto &[first, second] : request.knownLeads) {
    seed.assign(first, second);
  }
  const bool boardKnown = request.plugboard == PlugboardMode::Known;
  if (boardKnown) {
    for (int letter = 0; letter < alphabetSize; ++letter) {
      seed.assign(letter, seedMachine.getPlugboard().swap(letter));
    }
  }

  const auto first = request.cribOffset.value_or(0);
  const auto last = request.cribOffset.value_or(lastOffset);
  std::vector<Menu> menus;
  for (std::size_t offset = first; offset <= last; ++offset) {
    if (auto menu = makeMenu(ciphertext, crib, offset)) {
      menus.push_back(std::move(*menu));
    }
  }
  return Search(ciphertext, crib, settings, limits, stop, progress, candidate)
      .run(menus, last - first + 1, seed, boardKnown, searchProfiles(request));
}

} // namespace bombe
