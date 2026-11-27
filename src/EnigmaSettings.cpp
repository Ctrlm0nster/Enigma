#include "EnigmaSettings.hpp"
#include "Charactermap.hpp"

#include <stdexcept>

namespace enigma {
namespace {

struct RotorSpec {
  const char *wiring;
  char notch1;
  char notch2;
  bool steppable;
};

// Historical wirings: https://enigmamuseum.com/rotwirg.htm
constexpr std::array<RotorSpec, 10> allRotors{{
    {"EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q', '\0', true},   // I
    {"AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E', '\0', true},   // II
    {"BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V', '\0', true},   // III
    {"ESOVPZJAYQUIRHXLNFTGKDCMWB", 'J', '\0', true},   // IV
    {"VZBRGITYUPSDNHLXAWMJQOFECK", 'Z', '\0', true},   // V
    {"JPGVOUMFYQBENHZRDKASXLICTW", 'Z', 'M',  true},   // VI (Kriegsmarine)
    {"NZJHGRCXMYSWBOUFAIVLPEKQDT", 'Z', 'M',  true},   // VII (Kriegsmarine)
    {"FKQHTLXOCBJSPDZRAMEWNIUYGV", 'Z', 'M',  true},   // VIII (Kriegsmarine)
    {"LEYJVCNIXWPBQMDRTAKZGFUHOS", '\0', '\0', false}, // Beta (M4 Greek)
    {"FSOKANUERHMBTIYCWLQPZXVGJD", '\0', '\0', false}, // Gamma (M4 Greek)
}};

bool isContact(int contact) { return contact >= 0 && contact < 26; }

} // namespace

std::span<const HistoricalPreset> historicalPresets() {
  static const std::array<HistoricalPreset, 5> presets{{
    {"Operation Barbarossa", "22 Jun 1941",
     "Wehrmacht Heer frontline key at launch of Eastern Front (Rotors II-IV-V)",
     MachineSettings{
         .rotorCount = 3,
         .starts = {charToIndex('W'), charToIndex('X'), charToIndex('C'), 0, 0},
         .rings = {1, 20, 11, 0, 0}, // 02, 21, 12
         .plugPairs = {{charToIndex('A'), charToIndex('V')},
                       {charToIndex('B'), charToIndex('S')},
                       {charToIndex('C'), charToIndex('G')},
                       {charToIndex('D'), charToIndex('L')},
                       {charToIndex('F'), charToIndex('U')},
                       {charToIndex('H'), charToIndex('Z')},
                       {charToIndex('I'), charToIndex('N')},
                       {charToIndex('K'), charToIndex('M')},
                       {charToIndex('O'), charToIndex('W')},
                       {charToIndex('R'), charToIndex('X')}},
         .reflectorIndex = 1,
         .rotorTypes = {1, 3, 4, 0, 0} // II, IV, V
     }},
    {"U-552 Triton Shark", "1 Feb 1942",
     "Kriegsmarine M4 4-rotor operational key in North Atlantic patrol",
     MachineSettings{
         .rotorCount = 4,
         .starts = {charToIndex('V'), charToIndex('J'), charToIndex('Z'),
                    charToIndex('E'), 0},
         .rings = {0, 19, 10, 3, 0}, // 01, 20, 11, 04
         .plugPairs = {{charToIndex('A'), charToIndex('T')},
                       {charToIndex('B'), charToIndex('L')},
                       {charToIndex('D'), charToIndex('F')},
                       {charToIndex('G'), charToIndex('J')},
                       {charToIndex('H'), charToIndex('M')},
                       {charToIndex('N'), charToIndex('W')},
                       {charToIndex('O'), charToIndex('P')},
                       {charToIndex('Q'), charToIndex('Y')},
                       {charToIndex('R'), charToIndex('Z')},
                       {charToIndex('V'), charToIndex('X')}},
         .reflectorIndex = 3, // Thin B
         .rotorTypes = {8, 1, 3, 0, 0} // Beta, II, IV, I
     }},
    {"North Atlantic Wolfpack", "14 Dec 1943",
     "Kriegsmarine M4 4-rotor operational key with naval dual-notch Rotor VII",
     MachineSettings{
         .rotorCount = 4,
         .starts = {charToIndex('N'), charToIndex('A'), charToIndex('V'),
                    charToIndex('Y'), 0},
         .rings = {11, 4, 17, 21, 0}, // 12, 05, 18, 22
         .plugPairs = {{charToIndex('A'), charToIndex('N')},
                       {charToIndex('E'), charToIndex('Z')},
                       {charToIndex('H'), charToIndex('K')},
                       {charToIndex('I'), charToIndex('J')},
                       {charToIndex('L'), charToIndex('R')},
                       {charToIndex('M'), charToIndex('O')},
                       {charToIndex('Q'), charToIndex('S')},
                       {charToIndex('T'), charToIndex('U')},
                       {charToIndex('V'), charToIndex('W')},
                       {charToIndex('X'), charToIndex('Y')}},
         .reflectorIndex = 3, // Thin B
         .rotorTypes = {9, 6, 4, 2, 0} // Gamma, VII, V, III
     }},
    {"Normandy Invasion", "6 Jun 1944",
     "German Army Group B command circuit during Allied D-Day landings",
     MachineSettings{
         .rotorCount = 3,
         .starts = {charToIndex('D'), charToIndex('A'), charToIndex('Y'), 0, 0},
         .rings = {5, 13, 24, 0, 0}, // 06, 14, 25
         .plugPairs = {{charToIndex('C'), charToIndex('H')},
                       {charToIndex('E'), charToIndex('J')},
                       {charToIndex('K'), charToIndex('Q')},
                       {charToIndex('L'), charToIndex('M')},
                       {charToIndex('P'), charToIndex('W')},
                       {charToIndex('S'), charToIndex('T')},
                       {charToIndex('U'), charToIndex('Z')},
                       {charToIndex('B'), charToIndex('V')},
                       {charToIndex('A'), charToIndex('X')},
                       {charToIndex('D'), charToIndex('F')}},
         .reflectorIndex = 1, // Refl B
         .rotorTypes = {2, 0, 1, 0, 0} // III, I, II
     }},
    {"Scharnhorst Final", "26 Dec 1943",
     "Battle of the North Cape naval battle group cipher key (Rotors III-VI-VIII)",
     MachineSettings{
         .rotorCount = 3,
         .starts = {charToIndex('U'), charToIndex('Z'), charToIndex('K'), 0, 0},
         .rings = {0, 7, 12, 0, 0}, // 01, 08, 13
         .plugPairs = {{charToIndex('A'), charToIndex('N')},
                       {charToIndex('B'), charToIndex('O')},
                       {charToIndex('C'), charToIndex('P')},
                       {charToIndex('D'), charToIndex('Q')},
                       {charToIndex('E'), charToIndex('R')},
                       {charToIndex('F'), charToIndex('S')},
                       {charToIndex('G'), charToIndex('T')},
                       {charToIndex('H'), charToIndex('U')},
                       {charToIndex('I'), charToIndex('V')},
                       {charToIndex('J'), charToIndex('X')}},
         .reflectorIndex = 1, // Refl B
         .rotorTypes = {2, 5, 7, 0, 0} // III, VI, VIII
     }},
  }};
  return presets;
}

EnigmaMachine makeMachine(const MachineSettings &settings) {
  if (settings.rotorCount < minimumRotorCount ||
      settings.rotorCount > maximumRotorCount) {
    throw std::invalid_argument("Rotor count must be between 3 and 5");
  }
  if (settings.plugPairs.size() > plugLimit) {
    throw std::invalid_argument("The plugboard supports up to 10 leads");
  }

  std::vector<Rotor> wheels;
  wheels.reserve(settings.rotorCount);
  for (std::size_t i = 0; i < settings.rotorCount; ++i) {
    if (!isContact(settings.starts[i]) || !isContact(settings.rings[i])) {
      throw std::invalid_argument("Invalid rotor start or ring setting");
    }
    const int typeIdx = (settings.rotorTypes[i] >= 0 &&
                         settings.rotorTypes[i] < static_cast<int>(allRotors.size()))
                            ? settings.rotorTypes[i]
                            : static_cast<int>(i % allRotors.size());
    const auto &spec = allRotors[static_cast<std::size_t>(typeIdx)];
    auto &rotor = wheels.emplace_back(spec.wiring, spec.notch1, spec.notch2,
                                      settings.rings[i], spec.steppable);
    rotor.setPosition(settings.starts[i]);
  }

  Plugboard plugboard;
  for (const auto &[first, second] : settings.plugPairs) {
    if (!isContact(first) || !isContact(second) || first == second ||
        plugboard.swap(first) != first || plugboard.swap(second) != second) {
      throw std::invalid_argument(
          "Plugboard leads require distinct unused sockets");
    }
    plugboard.addWire(first, second);
  }

  static const std::array<Reflector, 5> allReflectors{
      Reflector("EJMZALYXVBWFCRQUONTSPIKHGD"), // A
      Reflector("YRUHQSLDPXNGOKMIEBFZCWVJAT"), // B
      Reflector("FVPJIAOYEDRZXWGCTKUQSBNMHL"), // C
      Reflector("ENKQAUYWJICOPBLMDXZVFTHRGS"), // Thin B (M4 Bruno)
      Reflector("RDOBJNTKVEHMLFCWZAXGYIPSUQ"), // Thin C (M4 Caesar)
  };
  const std::size_t reflIdx =
      (settings.reflectorIndex >= 0 &&
       settings.reflectorIndex < static_cast<int>(allReflectors.size()))
          ? static_cast<std::size_t>(settings.reflectorIndex)
          : 1;

  return EnigmaMachine(plugboard, std::move(wheels), allReflectors[reflIdx]);
}

} // namespace enigma
