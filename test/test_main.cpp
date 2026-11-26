#include "Charactermap.hpp"
#include "EnigmaMachine.hpp"
#include "workbench/Session.hpp"
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

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

TEST_CASE("Invalid wiring cannot create a broken rotor or reflector",
          "[enigma]") {
  CHECK_THROWS_AS(Rotor("AAAAAAAAAAAAAAAAAAAAAAAAAA", 'Q'),
                  std::invalid_argument);
  CHECK_THROWS_AS(Reflector("BCDEFGHIJKLMNOPQRSTUVWXYZA"),
                  std::invalid_argument);
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

TEST_CASE("Pasting formats letters without altering how the machine steps",
          "[workbench]") {
  workbench::Session pasted;
  workbench::Session typed;
  REQUIRE(pasted.paste("Enig-ma! 123") == 6);
  for (const char letter : std::string("ENIGMA")) {
    REQUIRE(typed.typeLetter(letter));
  }
  CHECK(pasted.input() == typed.input());
  CHECK(pasted.output() == typed.output());
  CHECK(pasted.positionAt(2) == typed.positionAt(2));

  CHECK(pasted.paste(std::string(workbench::messageLimit, 'z')) ==
        workbench::messageLimit - 6);
  CHECK(pasted.input().size() == workbench::messageLimit);
  CHECK(pasted.paste("MORE") == 0);
}

TEST_CASE("Pasting can reuse the session's own tape", "[workbench]") {
  const std::string message = "THEQUICKBROWNFOXJUMPSOVERTHELAZYDOG";
  workbench::Session session;
  REQUIRE(session.paste(message) == message.size());

  SECTION("Plaintext") {
    REQUIRE(session.paste(session.input()) == message.size());
    CHECK(session.input() == message + message);
  }

  SECTION("Ciphertext") {
    const std::string ciphertext = session.output();
    REQUIRE(session.paste(session.output()) == ciphertext.size());
    CHECK(session.input() == message + ciphertext);
  }
}

TEST_CASE("Changing the rotor count rebuilds the entire tape and trace",
          "[workbench]") {
  workbench::Session session;
  REQUIRE(session.rotorCount() == 3);
  REQUIRE(session.paste("The signal follows every rotor") == 26);
  const auto plaintext = session.input();
  const auto threeRotorOutput = session.output();

  REQUIRE(session.setRotorCount(4));
  CHECK(session.trace()->rotorForward.size() == 4);
  CHECK(session.trace()->rotorBackward.size() == 4);
  const auto fourRotorOutput = session.output();
  CHECK(fourRotorOutput != threeRotorOutput);

  REQUIRE(session.setRotorCount(5));
  CHECK(session.trace()->rotorForward.size() == 5);
  CHECK(session.trace()->rotorBackward.size() == 5);
  CHECK(session.output() != fourRotorOutput);
  session.adjustRotor(4, 1, 2);
  const auto adjustedOutput = session.output();
  CHECK_FALSE(session.setRotorCount(6));
  CHECK_FALSE(session.setRotorCount(2));
  CHECK(session.rotorCount() == 5);
  CHECK(session.output() == adjustedOutput);

  REQUIRE(session.setRotorCount(3));
  CHECK(session.output() == threeRotorOutput);
  REQUIRE(session.setRotorCount(5));
  CHECK(session.startAt(4) == 1);
  CHECK(session.ringAt(4) == 2);
  CHECK(session.output() == adjustedOutput);
  CHECK(session.input() == plaintext);

  session.reset();
  CHECK(session.rotorCount() == 3);
  CHECK(session.input().empty());
}

TEST_CASE("Four and five rotor configurations can decipher their output",
          "[workbench]") {
  const std::string message = "EVERYROTORSTEPCOUNTSWHENDECIPHERINGMESSAGES";
  for (std::size_t count = 4; count <= workbench::maximumRotorCount; ++count) {
    CAPTURE(count);
    workbench::Session sender;
    workbench::Session receiver;
    REQUIRE(sender.setRotorCount(count));
    REQUIRE(receiver.setRotorCount(count));
    sender.adjustRotor(count - 1, -1, 2);
    receiver.adjustRotor(count - 1, -1, 2);
    REQUIRE(sender.paste(message) == message.size());
    REQUIRE(receiver.paste(sender.output()) == message.size());
    CHECK(receiver.output() == message);
  }
}
