#include "Charactermap.hpp"
#include <cassert>

int charToIndex(char c) {
  assert(c >= 'A' && c <= 'Z');
  return c - 'A';
}

char indexToChar(int i) {
  assert(i >= 0 && i < 26);
  return static_cast<char>(i + 'A');
}

std::size_t idx(int i) {
  assert(i >= 0 && i < 26);
  return static_cast<std::size_t>(i);
}
