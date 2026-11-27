#include "EnigmaMachine.hpp"
#include "Charactermap.hpp"
#include <utility>

EnigmaMachine::EnigmaMachine(Plugboard p, std::vector<Rotor> r, Reflector ref)
    : plugboard(p), rotors(std::move(r)), reflector(ref) {}

const std::vector<Rotor> &EnigmaMachine::getRotors() const { return rotors; }

const Plugboard &EnigmaMachine::getPlugboard() const { return plugboard; }

void EnigmaMachine::stepRotors() {
  // Walk left to right so each notch check sees a rotor before it moves.
  for (std::size_t i = 0; i < rotors.size(); ++i) {
    if (i + 1 == rotors.size() || (i != 0 && rotors[i].atNotch()) ||
        rotors[i + 1].atNotch()) {
      rotors[i].step();
    }
  }
}

int EnigmaMachine::scramble(int signal, SignalTrace *trace) const {
  for (auto rotor = rotors.rbegin(); rotor != rotors.rend(); ++rotor) {
    signal = rotor->forward(signal);
    if (trace) {
      trace->rotorForward.push_back(indexToChar(signal));
    }
  }
  signal = reflector.reflect(signal);
  if (trace) {
    trace->reflected = indexToChar(signal);
  }
  for (const auto &rotor : rotors) {
    signal = rotor.backward(signal);
    if (trace) {
      trace->rotorBackward.push_back(indexToChar(signal));
    }
  }
  return signal;
}

EnigmaMachine::Permutation EnigmaMachine::scrambler() const {
  Permutation map{};
  for (int i = 0; i < 26; ++i) {
    map[idx(i)] = static_cast<std::uint8_t>(scramble(i, nullptr));
  }
  return map;
}

char EnigmaMachine::processChar(char c) {
  if (c < 'A' || c > 'Z') {
    return c;
  }
  stepRotors();
  const int signal = plugboard.swap(charToIndex(c));
  return indexToChar(plugboard.swap(scramble(signal, nullptr)));
}

SignalTrace EnigmaMachine::processCharWithTrace(char c) {
  SignalTrace trace;
  trace.input = c;

  if (c < 'A' || c > 'Z') {
    trace.plugboardForward = c;
    trace.reflected = c;
    trace.output = c;
    return trace;
  }

  stepRotors();

  int signal = charToIndex(c);

  signal = plugboard.swap(signal);
  trace.plugboardForward = indexToChar(signal);

  trace.rotorForward.reserve(rotors.size());
  trace.rotorBackward.reserve(rotors.size());
  signal = plugboard.swap(scramble(signal, &trace));
  trace.output = indexToChar(signal);

  return trace;
}

std::string EnigmaMachine::processString(const std::string &input) {
  std::string output;
  output.reserve(input.size());
  for (char c : input) {
    output.push_back(processChar(c));
  }
  return output;
}
