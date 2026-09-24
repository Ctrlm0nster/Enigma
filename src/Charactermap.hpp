#pragma once
#include <cassert>

// Input and Output
// Mapping character to Integer from A = 0 to Z = 25:
inline int charToIndex(char c) {
  assert(c >= 'A' && c <= 'Z');
  return c - 'A';
}

inline char indexToChar(int i) {
  assert(i >= 0 && i < 26);
  return static_cast<char>(i + 'A');
}

// Checked int -> array index conversion (single place for the cast)
// required for explicit converstion for std::array
constexpr std::size_t idx(int i) {
  assert(i >= 0 && i < 26);
  return static_cast<std::size_t>(i);
}
