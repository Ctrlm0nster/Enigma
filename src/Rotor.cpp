#include "Rotor.hpp"
#include "Charactermap.hpp"
#include <cassert>

Rotor::Rotor(const std::string &wiring, char notchChar, int initRingSetting)
    : notch(charToIndex(notchChar)), ringSetting(initRingSetting) {
  assert(wiring.size() == 26);
  for (int i = 0; i < 26; ++i) {
    int out = charToIndex(wiring[idx(i)]);
    forwardWiring[idx(i)] = out;
    backwardWiring[idx(out)] = i;
  }
}

void Rotor::setPosition(int p) { position = p; }
int Rotor::getPosition() const { return position; }
void Rotor::setRingSetting(int r) { ringSetting = r; }

bool Rotor::atNotch() const { return position == notch; }

void Rotor::step() { position = (position + 1) % 26; }

int Rotor::forward(int signal) const {
  int offset = (position - ringSetting + 26) % 26;
  int shifted = (signal + offset) % 26;
  int out = forwardWiring[idx(shifted)];
  return (out - offset + 26) % 26;
}

int Rotor::backward(int signal) const {
  int offset = (position - ringSetting + 26) % 26;
  int shifted = (signal + offset) % 26;
  int out = backwardWiring[idx(shifted)];
  return (out - offset + 26) % 26;
}
