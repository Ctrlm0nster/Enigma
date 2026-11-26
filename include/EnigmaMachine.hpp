/**
 * @file EnigmaMachine.hpp
 * @brief Complete Enigma machine simulator and signal trace engine.
 *
 * Assembles plugboard, rotor bank, and reflector into a coherent circuit,
 * executes pre-encryption stepping (including double-stepping anomaly),
 * and produces telemetry traces for UI visualization.
 */

#pragma once
#include "Plugboard.hpp"
#include "Reflector.hpp"
#include "Rotor.hpp"
#include <string>
#include <vector>

/**
 * @struct SignalTrace
 * @brief Captures the intermediate electrical state at each stage of encryption.
 *
 * Used by the UI lampboard and signal view to illustrate how a single keypress
 * travels through every component in real-time.
 */
struct SignalTrace {
  char input{};                      ///< Original uppercase key pressed.
  char plugboardForward{};           ///< Character exiting plugboard entering rotors.
  std::vector<char> rotorForward;    ///< Characters exiting each rotor (fast to slow).
  char reflected{};                  ///< Character exiting reflector.
  std::vector<char> rotorBackward;   ///< Characters exiting each rotor in reverse (slow to fast).
  char output{};                     ///< Final illuminated lampboard letter.
};

/**
 * @class EnigmaMachine
 * @brief Complete Enigma machine simulation chassis.
 *
 * The default three-rotor setup follows this path. With additional rotors,
 * the signal travels right to left through every rotor before reflection
 * and left to right on the way back.
 *
 * @mermaid
 * flowchart LR
 *     Key[Key In] --> Stepping[Rotor Stepping]
 *     Stepping --> PB1[Plugboard Forward]
 *     PB1 --> R3F[Rotor III Fast]
 *     R3F --> R2F[Rotor II Middle]
 *     R2F --> R1F[Rotor I Slow]
 *     R1F --> Ref[Reflector B]
 *     Ref --> R1B[Rotor I Reverse]
 *     R1B --> R2B[Rotor II Reverse]
 *     R2B --> R3B[Rotor III Reverse]
 *     R3B --> PB2[Plugboard Return]
 *     PB2 --> Lamp[Lampboard Out]
 * @endmermaid
 */
class EnigmaMachine {
private:
  Plugboard plugboard;         ///< Active plugboard.
  std::vector<Rotor> rotors;   ///< Ordered rotors (index 0 = slow/left, back = fast/right).
  Reflector reflector;         ///< Fixed reflector.

  /**
   * @brief Advances rotor positions according to Enigma mechanical pawls.
   *
   * The rightmost rotor advances on every letter. Each rotor at its notch,
   * except the leftmost rotor, also advances itself and its left neighbor.
   * Which rotors move is decided before any positions change.
   */
  void stepRotors();

public:
  /**
   * @brief Assembles an Enigma machine with plugboard, rotor sequence, and reflector.
   *
   * @param p Configured plugboard instance.
   * @param r Vector of rotors ordered left-to-right (index 0 is leftmost/slowest).
   * @param ref Configured reflector instance.
   */
  EnigmaMachine(Plugboard p, std::vector<Rotor> r, Reflector ref);

  /**
   * @brief Returns a read-only reference to the rotor vector.
   *
   * @return Const reference to internal rotor vector.
   */
  const std::vector<Rotor> &getRotors() const;

  /**
   * @brief Returns a read-only reference to the plugboard.
   *
   * @return Const reference to internal plugboard.
   */
  const Plugboard &getPlugboard() const;

  /**
   * @brief Encrypts or decrypts a single character.
   *
   * If @p c is an alphabetic character, advances rotors and scrambles the signal.
   * Non-alphabetic characters are returned unmodified without advancing rotors.
   *
   * @param c Plaintext or ciphertext character.
   * @return Encrypted or decrypted character.
   */
  char processChar(char c);

  /**
   * @brief Encrypts a character while recording each intermediate stage.
   *
   * @param c Character to encrypt.
   * @return SignalTrace containing states at each physical stage.
   */
  SignalTrace processCharWithTrace(char c);

  /**
   * @brief Processes an entire string sequentially.
   *
   * @param input Input text.
   * @return Processed output string.
   */
  std::string processString(const std::string &input);
};
