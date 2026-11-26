#pragma once
#include <array>

class Plugboard {
private:
  std::array<int, 26> wiring;

public:
  // Initializes a blank plugboard where every letter maps to itself.
  Plugboard();

  // Connects two distinct letters together on the plugboard via a wire.
  void addWire(int a, int b);

  // Swaps an electrical signal passing through the plugboard.
  int swap(int input) const;
};
