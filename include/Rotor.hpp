#pragma once
#include <array>
#include <string>

class Rotor {
private:
  std::array<int, 26> forwardWiring;
  std::array<int, 26> backwardWiring;
  int position = 0;
  int notch;
  int ringSetting = 0;

public:
  // Initializes a historical rotor with its wiring, turnover notch, and ring setting.
  Rotor(const std::string &wiring, char notchChar, int initRingSetting = 0);
  
  // Sets the current rotational position of the rotor (0-25).
  void setPosition(int p);

  // Gets the current rotational position of the rotor.
  int getPosition() const;

  // Sets the ring offset (Ringstellung).
  void setRingSetting(int r);
  
  // Checks if the rotor is currently resting at its turnover notch position.
  bool atNotch() const;

  // Mechanically advances the rotor position by one step.
  void step();
  
  // Passes an electrical signal forward (right-to-left) through the rotor's wiring.
  int forward(int signal) const;

  // Passes an electrical signal backward (left-to-right) through the inverse wiring.
  int backward(int signal) const;
};
