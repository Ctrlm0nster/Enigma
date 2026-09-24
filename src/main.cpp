#include "Charactermap.hpp"
#include "Plugboard.hpp"
#include <cassert>
#include <iostream>

// // Input and Output
// // Mapping character to Integer from A = 0 to Z = 25:
// int charToIndex(char c) {
//   assert(c >= 'A' && c <= 'Z');
//   return c - 'A';
// }
//
// char indexToChar(int i) {
//   assert(i >= 0 && i < 26);
//   return static_cast<char>(i + 'A');
// }
//
// // Checked int -> array index conversion (single place for the cast)
// // required for explicit converstion for std::array
// constexpr std::size_t idx(int i) {
//   assert(i >= 0 && i < 26);
//   return static_cast<std::size_t>(i);
// }
//
// // Rotor
class Rotor {
private:
  std::array<int, 26> forwardWiring;
  std::array<int, 26> backwardWiring;
  int position = 0;
  int notch;

public:
  Rotor(const std::string &wiring, char notchChar)
      : notch(charToIndex(notchChar)) {
    assert(wiring.size() == 26);
    for (int i = 0; i < 26; ++i) {
      int out = charToIndex(wiring[idx(i)]);
      forwardWiring[idx(i)] = out;
      backwardWiring[idx(out)] = i; // inverse mapping
    }
  }
  bool atNotch() const { return position == notch; }

  void step() { position = (position + 1) % 26; }

  int forward(int signal) const {
    int shifted = (signal + position) % 26;
    int out = forwardWiring[idx(shifted)];
    return (out - position + 26) % 26;
  }

  int backward(int signal) const {
    int shifted = (signal + position) % 26;
    int out = backwardWiring[idx(shifted)];
    return (out - position + 26) % 26;
  }
};

int main() {
  Plugboard parth;
  parth.addWire(charToIndex('A'), charToIndex('Z'));
  parth.addWire(charToIndex('B'), charToIndex('Y'));

  std::cout << indexToChar(parth.swap(charToIndex('A'))) << "\n"; // Z
  std::cout << indexToChar(parth.swap(charToIndex('Z'))) << "\n"; // A
  std::cout << indexToChar(parth.swap(charToIndex('C'))) << "\n"; // C (unwired)
  return 0;
}
