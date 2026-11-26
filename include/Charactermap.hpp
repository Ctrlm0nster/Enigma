/**
 * @file Charactermap.hpp
 * @brief Core utility functions for character-to-index mapping and signal arithmetic.
 *
 * Provides bidirectional conversions between uppercase ASCII letters ('A'..'Z')
 * and zero-indexed integers (0..25) used throughout the cryptographic simulation.
 */

#pragma once
#include <cstddef>

/**
 * @brief Converts an uppercase ASCII character ('A'-'Z') to a 0-indexed integer (0-25).
 *
 * @param c Uppercase alphabetic character in the range ['A', 'Z'].
 * @return Integer index where 'A' -> 0, 'B' -> 1, ..., 'Z' -> 25.
 * @note Asserts at runtime that @p c is within ['A', 'Z'].
 */
int charToIndex(char c);

/**
 * @brief Converts a 0-indexed integer (0-25) back to an uppercase ASCII character ('A'-'Z').
 *
 * @param i Integer index in the range [0, 25].
 * @return Uppercase ASCII character where 0 -> 'A', 1 -> 'B', ..., 25 -> 'Z'.
 * @note Asserts at runtime that @p i is within [0, 25].
 */
char indexToChar(int i);

/**
 * @brief Safely casts a 0-indexed integer signal into a std::size_t for array subscripting.
 *
 * Used throughout wiring lookups and container operations to ensure bounds compliance.
 *
 * @param i Integer signal in the range [0, 25].
 * @return Equivalent std::size_t value.
 * @note Asserts at runtime that @p i is within [0, 25].
 */
std::size_t idx(int i);
