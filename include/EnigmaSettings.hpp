#pragma once

#include "EnigmaMachine.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace enigma {

inline constexpr std::array rotorNames{"I", "II", "III", "IV", "V",
                                       "VI", "VII", "VIII", "Beta", "Gamma"};
inline constexpr std::array reflectorNames{"Refl. A", "Refl. B", "Refl. C",
                                           "Thin B", "Thin C"};

inline constexpr std::size_t minimumRotorCount = 3;
inline constexpr std::size_t maximumRotorCount = 5;
inline constexpr std::size_t messageLimit = 240;
inline constexpr std::size_t plugLimit = 10;

using PlugPair = std::pair<int, int>;

struct MachineSettings {
  std::size_t rotorCount = minimumRotorCount;
  std::array<int, maximumRotorCount> starts{};
  std::array<int, maximumRotorCount> rings{};
  std::vector<PlugPair> plugPairs;
  int reflectorIndex = 1; ///< 0=A, 1=B, 2=C, 3=Thin B, 4=Thin C
  std::array<int, maximumRotorCount> rotorTypes{0, 1, 2, 3, 4}; ///< Index into rotorNames
};

struct HistoricalPreset {
  std::string_view name;
  std::string_view date;
  std::string_view description;
  MachineSettings settings;
};

std::span<const HistoricalPreset> historicalPresets();

/** Creates a fresh machine from Workbench settings with offsets in [0, 25].
 *  Throws std::invalid_argument for unsupported counts or invalid settings.
 */
EnigmaMachine makeMachine(const MachineSettings &settings);

/** Returns an uppercase ASCII letter or zero for any other input byte. */
constexpr char normalizeLetter(char character) {
  if (character >= 'a' && character <= 'z') {
    return static_cast<char>(character + ('A' - 'a'));
  }
  return character >= 'A' && character <= 'Z' ? character : '\0';
}

} // namespace enigma
