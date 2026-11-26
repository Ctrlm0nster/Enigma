#pragma once
#include "Plugboard.hpp"
#include "Reflector.hpp"
#include "Rotor.hpp"
#include <string>
#include <vector>

struct SignalTrace {
  char input{};
  char plugboardForward{};
  std::vector<char> rotorForward;
  char reflected{};
  std::vector<char> rotorBackward;
  char output{};
};

class EnigmaMachine {
private:
  Plugboard plugboard;
  std::vector<Rotor> rotors;
  Reflector reflector;

  // Mechanically advances the necessary rotors prior to processing a keystroke.
  void stepRotors();

public:
  // Assembles the Enigma Machine with a plugboard, sequence of rotors, and a
  // reflector.
  EnigmaMachine(Plugboard p, std::vector<Rotor> r, Reflector ref);

  // Returns a read-only reference to the machine's rotors (useful for UI
  // inspection).
  const std::vector<Rotor> &getRotors() const;

  // Returns a read-only reference to the active plugboard.
  const Plugboard &getPlugboard() const;

  // Processes a single character through the entire Enigma encryption circuit.
  char processChar(char c);

  // Processes one character and records every stage for UI visualization.
  SignalTrace processCharWithTrace(char c);

  // Processes a full string character-by-character.
  std::string processString(const std::string &input);
};
