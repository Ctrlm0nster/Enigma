#include "EnigmaSettings.hpp"

#include <stdexcept>

namespace enigma {
namespace {

struct RotorSpec {
  const char *wiring;
  char notch;
};

// https://enigmamuseum.com/rotwirg.htm
constexpr std::array<RotorSpec, maximumRotorCount> rotors{{
    {"EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q'},
    {"AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E'},
    {"BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V'},
    {"ESOVPZJAYQUIRHXLNFTGKDCMWB", 'J'},
    {"VZBRGITYUPSDNHLXAWMJQOFECK", 'Z'},
}};
const Reflector reflectorB("YRUHQSLDPXNGOKMIEBFZCWVJAT");

bool isContact(int contact) { return contact >= 0 && contact < 26; }

} // namespace

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
    auto &rotor = wheels.emplace_back(rotors[i].wiring, rotors[i].notch,
                                      settings.rings[i]);
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
  return EnigmaMachine(plugboard, std::move(wheels), reflectorB);
}

} // namespace enigma
