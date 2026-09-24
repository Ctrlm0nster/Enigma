#pragma once
#include "Charactermap.hpp"
#include <array>
#include <cassert>
#include <numeric>

// Plugboard
class Plugboard {
private:
  std::array<int, 26> wiring;

public:
  Plugboard() {
    std::iota(wiring.begin(), wiring.end(), 0); // 0, 1, 2, ... 25 (identity)
  }

  void addWire(int a, int b) {
    assert(a != b);
    assert(wiring[idx(a)] == a && wiring[idx(b)] == b); // both unwired
    wiring[idx(a)] = b;
    wiring[idx(b)] = a;
  }

  int swap(int input) const {
    assert(input >= 0 && input < 26);
    return wiring[idx(input)]; // unwired letters map to themselves
  }
};
