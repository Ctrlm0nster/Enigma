/**
 * @file Plugboard.hpp
 * @brief Steckerbrett (plugboard) component simulation.
 *
 * Simulates the physical plugboard at the front of the Enigma machine.
 * Allows swapping pairs of letters before the signal reaches the rotors and
 * again after it emerges from the rotors.
 */

#pragma once
#include <array>

/**
 * @class Plugboard
 * @brief Models the Enigma plugboard (Steckerbrett).
 *
 * Holds reciprocal pair wiring for up to 10 connections (20 letters),
 * leaving unwired letters connected straight through (identity mapping).
 *
 * @mermaid
 * graph LR
 *     In[Input Signal] --> PB{Plugboard}
 *     PB -->|Unpaired letter| Out1[Self: A -> A]
 *     PB -->|Paired lead| Out2[Swapped: A <-> Z]
 * @endmermaid
 */
class Plugboard {
private:
  std::array<int, 26> wiring; ///< Mapping array where wiring[i] stores the swapped signal index.

public:
  /**
   * @brief Initializes a blank plugboard with identity mapping (0->0, 1->1, ..., 25->25).
   */
  Plugboard();

  /**
   * @brief Connects two distinct letters together via a patch lead.
   *
   * The connection is strictly symmetric: letter @p a maps to @p b and @p b maps to @p a.
   *
   * @param a 0-indexed index of first letter [0..25].
   * @param b 0-indexed index of second letter [0..25].
   * @note Asserts that @p a != @p b and that neither letter is already wired.
   */
  void addWire(int a, int b);

  /**
   * @brief Swaps an electrical signal passing through the plugboard.
   *
   * @param input 0-indexed integer signal [0..25].
   * @return Swapped signal index, or @p input if the letter has no patch lead attached.
   */
  int swap(int input) const;
};
