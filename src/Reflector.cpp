#include "Reflector.hpp"
#include "Charactermap.hpp"
#include <cassert>
#include <stdexcept>

Reflector::Reflector(const std::string &wiringConfig) {
  if (wiringConfig.size() != wiring.size()) {
    throw std::invalid_argument("Reflector wiring must contain 26 letters");
  }
  for (int i = 0; i < 26; ++i) {
    const auto contact = idx(i);
    const char letter = wiringConfig[contact];
    if (letter < 'A' || letter > 'Z') {
      throw std::invalid_argument(
          "Reflector wiring must use uppercase ASCII letters");
    }
    const int out = charToIndex(letter);
    if (out == i || wiringConfig[idx(out)] != indexToChar(i)) {
      throw std::invalid_argument(
          "Reflector wiring must connect distinct reciprocal pairs");
    }
    wiring[contact] = out;
  }
}

int Reflector::reflect(int signal) const {
  assert(signal >= 0 && signal < 26);
  return wiring[idx(signal)];
}
