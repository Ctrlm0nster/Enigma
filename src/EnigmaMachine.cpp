#include "EnigmaMachine.hpp"
#include "Charactermap.hpp"
#include <utility>

EnigmaMachine::EnigmaMachine(Plugboard p, std::vector<Rotor> r, Reflector ref)
    : plugboard(p), rotors(std::move(r)), reflector(ref) {}

const std::vector<Rotor> &EnigmaMachine::getRotors() const { return rotors; }

const Plugboard &EnigmaMachine::getPlugboard() const { return plugboard; }

void EnigmaMachine::stepRotors() {
  if (rotors.empty())
    return;

  std::size_t n = rotors.size();
  std::vector<bool> step(n, false);

  step[n - 1] = true;

  for (std::size_t i = n - 1; i > 0; --i) {
    if (rotors[i].atNotch()) {
      step[i - 1] = true;
      step[i] = true;
    }
  }

  for (std::size_t i = 0; i < n; ++i) {
    if (step[i]) {
      rotors[i].step();
    }
  }
}

char EnigmaMachine::processChar(char c) {
  return processCharWithTrace(c).output;
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
  for (std::size_t i = rotors.size(); i-- > 0;) {
    signal = rotors[i].forward(signal);
    trace.rotorForward.push_back(indexToChar(signal));
  }

  signal = reflector.reflect(signal);
  trace.reflected = indexToChar(signal);

  trace.rotorBackward.reserve(rotors.size());
  for (std::size_t i = 0; i < rotors.size(); ++i) {
    signal = rotors[i].backward(signal);
    trace.rotorBackward.push_back(indexToChar(signal));
  }

  signal = plugboard.swap(signal);
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
