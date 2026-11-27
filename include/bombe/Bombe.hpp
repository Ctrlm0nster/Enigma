#pragma once

#include "EnigmaSettings.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

namespace bombe {

inline constexpr std::size_t rotorCount = enigma::minimumRotorCount;
inline constexpr std::size_t startCount = 26 * 26 * 26;

/** Deciphers ASCII letters with a fresh machine. Throws on empty or long input.
 */
std::string decipher(std::string_view ciphertext,
                     const enigma::MachineSettings &settings);

enum class PlugboardMode { Known, Partial, Unknown };

struct SearchRequest {
  std::string ciphertext;
  std::string crib;
  std::optional<std::size_t> cribOffset; ///< Zero-based in the normalized text.
  std::array<int, rotorCount>
      rings{}; ///< Zero-based offsets, ordered left to right.
  PlugboardMode plugboard = PlugboardMode::Unknown;
  std::vector<enigma::PlugPair> knownLeads;
};

struct SearchLimits {
  std::size_t work = 2'000'000; ///< Settings and plugboard hypotheses combined.
  std::size_t results = 100;
};

enum class SearchStatus { Complete, Cancelled, WorkLimit, ResultLimit };

struct SearchProgress {
  std::size_t alignments = 0;
  std::size_t viableAlignments = 0;
  std::size_t settingsChecked = 0;
  std::size_t work = 0;
  std::size_t matches = 0;
};

struct Candidate {
  enigma::MachineSettings settings;
  std::size_t cribOffset = 0;
  std::string unresolvedSockets; ///< Assumed unplugged for this preview.
  std::string plaintext;
};

struct SearchResult {
  SearchStatus status = SearchStatus::Complete;
  SearchProgress progress;
  std::vector<Candidate> candidates;
};

using ProgressCallback = std::function<void(const SearchProgress &)>;

/** Searches the I–II–III profile with Reflector B and the specified rings.
 *  The callback runs on the caller's thread. Each candidate is replayed
 *  through Enigma before being returned. Throws on invalid requests or limits.
 */
SearchResult search(const SearchRequest &request, SearchLimits limits = {},
                    std::stop_token stop = {},
                    const ProgressCallback &progress = {});

} // namespace bombe
