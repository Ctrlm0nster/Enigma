#include "Rotor.hpp"
#include "Charactermap.hpp"
#include <stdexcept>

Rotor::Rotor(const std::string &wiring, char notchChar, int initRingSetting)
    : ringSetting(initRingSetting) {
  if (wiring.size() != forwardWiring.size()) {
    throw std::invalid_argument("Rotor wiring must contain 26 letters");
  }
  if (notchChar < 'A' || notchChar > 'Z') {
    throw std::invalid_argument("Rotor notch must be an uppercase ASCII letter");
  }
  notch = charToIndex(notchChar);

  backwardWiring.fill(-1);
  for (int i = 0; i < 26; ++i) {
    const auto contact = idx(i);
    const char letter = wiring[contact];
    if (letter < 'A' || letter > 'Z') {
      throw std::invalid_argument("Rotor wiring must use uppercase ASCII letters");
    }
    const int out = charToIndex(letter);
    auto &inverse = backwardWiring[idx(out)];
    if (inverse != -1) {
      throw std::invalid_argument("Rotor wiring must use each letter once");
    }
    forwardWiring[contact] = out;
    inverse = i;
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
