#pragma once
#include <array>
#include <string>

class Reflector {
private:
  std::array<int, 26> wiring;

public:
  // Initializes the reflector with a 26-character historical wiring layout.
  explicit Reflector(const std::string &wiringConfig);

  // Bounces an electrical signal back through the rotors using the fixed wiring.
  int reflect(int signal) const;
};
