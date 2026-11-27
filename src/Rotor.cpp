#include "Rotor.hpp"
#include "Charactermap.hpp"
#include <stdexcept>

Rotor::Rotor(const std::string &wiring, char notchChar, int initRingSetting)
    : Rotor(wiring, notchChar, '\0', initRingSetting, true) {
  if (notchChar < 'A' || notchChar > 'Z') {
    throw std::invalid_argument("Rotor notch must be an uppercase ASCII letter");
  }
}

Rotor::Rotor(const std::string &wiring, char notch1Char, char notch2Char,
             int initRingSetting, bool canStep)
    : ringSetting(initRingSetting), steppable(canStep) {
  if (wiring.size() != forwardWiring.size()) {
    throw std::invalid_argument("Rotor wiring must contain 26 letters");
  }
  notch1 = (notch1Char >= 'A' && notch1Char <= 'Z') ? charToIndex(notch1Char) : -1;
  notch2 = (notch2Char >= 'A' && notch2Char <= 'Z') ? charToIndex(notch2Char) : -1;

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

bool Rotor::atNotch() const {
  return (notch1 != -1 && position == notch1) ||
         (notch2 != -1 && position == notch2);
}

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
