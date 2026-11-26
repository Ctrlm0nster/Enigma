# Rotor Documentation

The `Rotor` (or Walzen) is the heart of the Enigma machine's cryptography. Rotors consist of complex internal cross-wirings that scramble the electrical signal. Furthermore, they physically rotate, changing the wiring configuration with every keystroke.

## Class Overview
Each rotor maintains a `forwardWiring` and a `backwardWiring` array (since signals pass through them twice: once on the way to the reflector, and once on the way back). They also keep track of their mechanical `position`, their designated turnover `notch`, and their `ringSetting` offset.

## Methods

### `Rotor(const std::string &wiring, char notchChar, int initRingSetting = 0)`
- **Description**: Constructor for initializing a historical rotor.
- **Parameters**:
  - `wiring` (const std::string &): A 26-character string detailing the internal cross-wiring from A to Z.
  - `notchChar` (char): The physical character on the rotor ring that engages the pawl, causing the adjacent rotor to step.
  - `initRingSetting` (int, default 0): The Ringstellung. This offsets the internal wiring relative to the physical letters printed on the rotor's exterior.
- **Behavior**: Calculates the forward wiring directly from the input string, and calculates the `backwardWiring` via inverse mapping to allow the signal to flow in reverse efficiently.

### `void setPosition(int p)`
- **Description**: Manually sets the current visible rotational position of the rotor.
- **Parameters**: `p` (int), the 0-indexed position.

### `int getPosition() const`
- **Description**: Retrieves the current rotational position. Useful for the UI to display the active state.

### `void setRingSetting(int r)`
- **Description**: Modifies the rotor's ring setting offset.

### `bool atNotch() const`
- **Description**: Checks if the rotor is currently positioned at its notch.
- **Returns**: `true` if the current `position` matches the `notch`, `false` otherwise.

### `void step()`
- **Description**: Mechanically rotates the rotor by one position.
- **Behavior**: Increments the `position` by 1. Uses modulo arithmetic (`% 26`) to wrap back to `0` after `25` (Z -> A).

### `int forward(int signal) const`
- **Description**: Passes an electrical signal through the rotor in the forward direction (Right to Left in the machine).
- **Parameters**: `signal` (int) between `0` and `25`.
- **Behavior**: 
  1. Calculates the `offset` by accounting for the current `position` and the `ringSetting`.
  2. Applies this offset to the incoming signal to determine the physical contact point.
  3. Looks up the destination contact in `forwardWiring`.
  4. Removes the offset from the output to translate the signal back to absolute machine coordinates.
- **Returns**: The scrambled 0-indexed signal.

### `int backward(int signal) const`
- **Description**: Passes the signal through the rotor in the reverse direction (Left to Right, post-reflector).
- **Behavior**: Functions identically to `forward()`, but uses the inverse `backwardWiring` array.
