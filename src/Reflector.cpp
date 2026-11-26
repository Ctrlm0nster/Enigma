#include "Reflector.hpp"
#include "Charactermap.hpp"
#include <cassert>

Reflector::Reflector(const std::string &wiringConfig) {
  assert(wiringConfig.size() == 26);
  for (int i = 0; i < 26; ++i) {
    int out = charToIndex(wiringConfig[idx(i)]);
    assert(out != i);
    wiring[idx(i)] = out;
  }
}

int Reflector::reflect(int signal) const {
  assert(signal >= 0 && signal < 26);
  return wiring[idx(signal)];
}
