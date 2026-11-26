#include "Plugboard.hpp"
#include "Charactermap.hpp"
#include <cassert>
#include <numeric>

Plugboard::Plugboard() {
  std::iota(wiring.begin(), wiring.end(), 0);
}

void Plugboard::addWire(int a, int b) {
  assert(a != b);
  assert(wiring[idx(a)] == a && wiring[idx(b)] == b);
  wiring[idx(a)] = b;
  wiring[idx(b)] = a;
}

int Plugboard::swap(int input) const {
  assert(input >= 0 && input < 26);
  return wiring[idx(input)];
}
