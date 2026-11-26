/**
 * @file Reflector.hpp
 * @brief Umkehrwalze (reflector) component simulation.
 *
 * Simulates the fixed reflector drum (e.g., Reflector B) at the left end of the
 * Enigma rotor basket that loops the electrical signal back through the rotors in reverse.
 */

#pragma once
#include <array>
#include <string>

/**
 * @class Reflector
 * @brief Models the Enigma reflector (Umkehrwalze).
 *
 * The reflector establishes 13 involutional pairs such that no letter can ever
 * reflect to itself (a property historically exploited by cryptanalysts).
 *
 * @mermaid
 * graph LR
 *     RotorIn[Signal from Rotor I] --> Ref[Reflector B]
 *     Ref -->|Fixed Scramble| RotorOut[Signal back to Rotor I]
 * @endmermaid
 */
class Reflector {
private:
  std::array<int, 26> wiring; ///< Fixed internal wire connections.

public:
  /**
   * @brief Constructs a reflector given a 26-character layout string.
   *
   * For example, Reflector B uses `"YRUHQSLDPXNGOKMIEBFZCWVJAT"`.
   *
   * @param wiringConfig A permutation of A–Z representing 13 reciprocal pairs.
   * @throws std::invalid_argument If the wiring does not form 13 distinct pairs.
   */
  explicit Reflector(const std::string &wiringConfig);

  /**
   * @brief Bounces an electrical signal through the reflector's internal wiring.
   *
   * @param signal 0-indexed incoming signal from the leftmost rotor [0..25].
   * @return 0-indexed reflected signal index [0..25].
   */
  int reflect(int signal) const;
};
