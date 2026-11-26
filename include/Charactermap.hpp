#pragma once
#include <cstddef>

// Converts an uppercase character ('A'-'Z') to a 0-indexed integer (0-25).
int charToIndex(char c);

// Converts a 0-indexed integer (0-25) back to an uppercase character ('A'-'Z').
char indexToChar(int i);

// Safely casts a 0-indexed integer to a std::size_t for array indexing.
std::size_t idx(int i);
