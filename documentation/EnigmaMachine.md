# EnigmaMachine Documentation

The `EnigmaMachine` acts as the central chassis and orchestrator for the entire simulation. It holds all the individual components (plugboard, rotors, reflector) together, manages the mechanical stepping of the rotors, and routes the electrical signal through the complete circuit.

## Class Overview
Maintains private instances of the `Plugboard`, a `std::vector<Rotor>`, and the `Reflector`.

## Methods

### `EnigmaMachine(Plugboard p, std::vector<Rotor> r, Reflector ref)`
- **Description**: Constructor to assemble a fully configured Enigma Machine.
- **Parameters**:
  - `p` (Plugboard): The configured plugboard instance.
  - `r` (std::vector<Rotor>): A vector of initialized rotors. The rightmost rotor (fastest) is at the back of the vector, and the leftmost rotor (slowest, adjacent to the reflector) is at the front of the vector.
  - `ref` (Reflector): The configured reflector instance.

### `const std::vector<Rotor>& getRotors() const`
- **Description**: Retrieves a read-only reference to the machine's rotors. Used primarily by the UI to render the current physical state/positions of the rotors.

### `const Plugboard& getPlugboard() const`
- **Description**: Retrieves a read-only reference to the plugboard. Used by the UI to render active wire connections.

### `void stepRotors()`
- **Description**: Handles the complex mechanical stepping of the rotors before a keypress is electrically processed.
- **Behavior**:
  1. The rightmost rotor (fast rotor) *always* steps on every keypress.
  2. If a rotor is at its `notch` position, it will engage the pawl of the rotor to its left, causing that left adjacent rotor to step as well.
  3. **Double-Stepping Anomaly**: Due to the historical mechanical design of the Enigma's pawls and ratchets, if the middle rotor is at its notch and is pushed by the right rotor, it will actually be pushed *again* on the very next keypress. This code explicitly models this anomaly by pushing both `i-1` and `i` when a notch is engaged.

### `char processChar(char c)`
- **Description**: Encrypts or decrypts a single character.
- **Parameters**: `c` (char): The plaintext or ciphertext letter.
- **Behavior**:
  1. Rejects non-alphabetic characters (returns them unchanged).
  2. Calls `stepRotors()` to advance the mechanical state.
  3. Converts the character to a 0-indexed signal (`charToIndex`).
  4. Passes the signal through the `Plugboard` (`swap`).
  5. Passes the signal `forward` through all `Rotors` (iterating right-to-left).
  6. Bounces the signal through the `Reflector` (`reflect`).
  7. Passes the signal `backward` through all `Rotors` (iterating left-to-right).
  8. Passes the signal through the `Plugboard` one final time (`swap`).
  9. Converts the resulting integer signal back to a `char` (`indexToChar`).
- **Returns**: The fully encrypted/decrypted output character.

### `std::string processString(const std::string& input)`
- **Description**: A convenience function for processing full sentences.
- **Behavior**: Iterates over every character in the `input` string, calling `processChar` on each one sequentially, and concatenating the results into a new output string.
