#include "Charactermap.hpp"
#include "EnigmaMachine.hpp"
#include "workbench/Session.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Enigma Reversibility", "[enigma]") {
  Rotor rotor1("EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q');
  Rotor rotor2("AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E');
  Rotor rotor3("BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V');
  rotor1.setPosition(charToIndex('A'));
  rotor2.setPosition(charToIndex('B'));
  rotor3.setPosition(charToIndex('C'));

  std::vector<Rotor> rotors;
  rotors.push_back(rotor1);
  rotors.push_back(rotor2);
  rotors.push_back(rotor3);

  Reflector reflector("YRUHQSLDPXNGOKMIEBFZCWVJAT");
  Plugboard plugboard;
  plugboard.addWire(charToIndex('A'), charToIndex('Z'));
  plugboard.addWire(charToIndex('B'), charToIndex('Y'));

  EnigmaMachine enigma(plugboard, rotors, reflector);

  std::string plaintext = "HELLOWORLDTHISISATEST";
  std::string ciphertext = enigma.processString(plaintext);

  // Reset
  rotor1.setPosition(charToIndex('A'));
  rotor2.setPosition(charToIndex('B'));
  rotor3.setPosition(charToIndex('C'));

  std::vector<Rotor> resetRotors;
  resetRotors.push_back(rotor1);
  resetRotors.push_back(rotor2);
  resetRotors.push_back(rotor3);

  EnigmaMachine enigmaDecrypt(plugboard, resetRotors, reflector);
  std::string decrypted = enigmaDecrypt.processString(ciphertext);

  REQUIRE(plaintext == decrypted);
}

TEST_CASE("Signal traces expose each encryption stage", "[enigma][trace]") {
  Rotor rotor1("EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q');
  Rotor rotor2("AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E');
  Rotor rotor3("BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V');
  std::vector<Rotor> rotors{rotor1, rotor2, rotor3};

  Plugboard plugboard;
  plugboard.addWire(charToIndex('A'), charToIndex('Z'));
  Reflector reflector("YRUHQSLDPXNGOKMIEBFZCWVJAT");

  EnigmaMachine tracedMachine(plugboard, rotors, reflector);
  EnigmaMachine referenceMachine(plugboard, rotors, reflector);
  const SignalTrace trace = tracedMachine.processCharWithTrace('A');

  REQUIRE(trace.input == 'A');
  REQUIRE(trace.plugboardForward == 'Z');
  REQUIRE(trace.rotorForward.size() == 3);
  REQUIRE(trace.rotorBackward.size() == 3);
  REQUIRE(trace.output == referenceMachine.processChar('A'));
}

TEST_CASE("Workbench edits replay the message from its configured start",
          "[workbench]") {
  workbench::Session edited;
  for (char letter : std::string("ENIGMA")) {
    REQUIRE(edited.typeLetter(letter));
  }
  edited.backspace();
  edited.adjustRotor(2, -1, 1);
  REQUIRE(edited.selectPlug(charToIndex('C')) ==
          workbench::PlugAction::Selected);
  REQUIRE(edited.selectPlug(charToIndex('A')) ==
          workbench::PlugAction::Connected);
  REQUIRE_FALSE(edited.isPlugConnected(charToIndex('Z')));

  workbench::Session fresh;
  fresh.adjustRotor(2, -1, 1);
  REQUIRE(fresh.selectPlug(charToIndex('A')) == workbench::PlugAction::Removed);
  REQUIRE(fresh.selectPlug(charToIndex('A')) ==
          workbench::PlugAction::Selected);
  REQUIRE(fresh.selectPlug(charToIndex('C')) ==
          workbench::PlugAction::Connected);
  for (char letter : std::string("ENIGM")) {
    REQUIRE(fresh.typeLetter(letter));
  }
  REQUIRE(edited.input() == fresh.input());
  REQUIRE(edited.output() == fresh.output());
  REQUIRE(edited.positionAt(2) == fresh.positionAt(2));
  REQUIRE(edited.trace()->input == 'M');
}
