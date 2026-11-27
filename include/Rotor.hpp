/**
 * @file Rotor.hpp
 * @brief Walzen (rotor / scrambler wheel) component simulation.
 *
 * Models a single historical Enigma rotor with forward/reverse internal cross-wiring,
 * adjustable core position, turnover notch, and ring setting (Ringstellung).
 */

#pragma once
#include <array>
#include <string>

/**
 * @class Rotor
 * @brief Simulates a single rotating cipher wheel.
 *
 * Handles relative contact shifts caused by current rotor position and ring setting,
 * forward signal flow, reverse signal flow via precomputed inverse mappings, and
 * mechanical turnover notch detection.
 *
 * @mermaid
 * sequenceDiagram
 *     autonumber
 *     actor Key as Keyboard Signal
 *     participant R as Rotor
 *     Note over R: Apply (Position - RingSetting) Offset
 *     Key->>R: Forward Signal
 *     R-->>Key: Shifted Pin Output
 * @endmermaid
 */
class Rotor {
private:
  std::array<int, 26> forwardWiring;  ///< Right-to-left wiring table.
  std::array<int, 26> backwardWiring; ///< Left-to-right inverse wiring table.
  int position = 0;                   ///< Current rotational position (0..25, displayed as A..Z).
  int notch1 = -1;                    ///< Primary turnover notch position (0..25, or -1 if none).
  int notch2 = -1;                    ///< Secondary turnover notch position (0..25, or -1 if none).
  int ringSetting = 0;                ///< Ringstellung offset (0..25).
  bool steppable = true;              ///< Whether this rotor steps during normal operation (false for Greek rotors).

public:
  /**
   * @brief Initializes a rotor with its internal wiring, turnover notch, and ring setting.
   *
   * @param wiring A permutation of A–Z specifying forward contacts.
   * @param notchChar Character marking the turnover notch (e.g., 'Q' for Rotor I).
   * @param initRingSetting Ringstellung offset [0..25], defaults to 0 ('A'/01).
   * @throws std::invalid_argument If the wiring or turnover notch is invalid.
   */
  Rotor(const std::string &wiring, char notchChar, int initRingSetting = 0);

  /**
   * @brief Initializes a rotor with optional dual turnover notches (naval rotors VI-VIII) or non-stepping (Greek Beta/Gamma).
   *
   * @param wiring A permutation of A–Z specifying forward contacts.
   * @param notch1Char Character marking first turnover notch, or '\0' for none.
   * @param notch2Char Character marking second turnover notch, or '\0' for none.
   * @param initRingSetting Ringstellung offset [0..25], defaults to 0 ('A'/01).
   * @param canStep Whether this rotor steps mechanically during typing.
   */
  Rotor(const std::string &wiring, char notch1Char, char notch2Char,
        int initRingSetting = 0, bool canStep = true);
  
  /**
   * @brief Returns whether this rotor steps mechanically during cipher operation.
   */
  bool isSteppable() const { return steppable; }
  
  /**
   * @brief Manually sets the visible rotational position of the rotor.
   *
   * @param p 0-indexed position [0..25].
   */
  void setPosition(int p);

  /**
   * @brief Gets the current rotational position of the rotor.
   *
   * @return Integer index [0..25].
   */
  int getPosition() const;

  /**
   * @brief Sets the ring offset (Ringstellung).
   *
   * Offsets the internal cross-wiring relative to the alphabet ring.
   *
   * @param r Ring setting [0..25].
   */
  void setRingSetting(int r);
  
  /**
   * @brief Checks if the rotor is resting at its mechanical turnover notch.
   *
   * @return true if the current position triggers stepping on the adjacent rotor.
   */
  bool atNotch() const;

  /**
   * @brief Mechanically advances the rotor by one step (modulo 26).
   */
  void step();
  
  /**
   * @brief Passes an electrical signal forward (right-to-left) through the rotor.
   *
   * @param signal 0-indexed input contact [0..25].
   * @return 0-indexed output contact [0..25].
   */
  int forward(int signal) const;

  /**
   * @brief Passes an electrical signal backward (left-to-right) through the inverse wiring.
   *
   * @param signal 0-indexed input contact from reflector direction [0..25].
   * @return 0-indexed output contact [0..25].
   */
  int backward(int signal) const;
};
