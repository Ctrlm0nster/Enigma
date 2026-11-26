# Reflector Documentation

The `Reflector` (or Umkehrwalze) is the final component in the Enigma signal path before the signal travels backward through the rotors. It takes an incoming signal and rigidly routes it to a different output wire. This component is what guarantees the Enigma machine's reversibility (encrypting and decrypting are the exact same operation).

## Class Overview
The `Reflector` class maintains a single `std::array<int, 26>` to represent its internal, immutable wiring.

## Methods

### `explicit Reflector(const std::string &wiringConfig)`
- **Description**: Constructor that builds a reflector based on a 26-character layout string (e.g., `"YRUHQSLDPXNGOKMIEBFZCWVJAT"` for Reflector B).
- **Parameters**:
  - `wiringConfig` (const std::string &): A 26-character string representing the output of the reflector when an input is fed in sequentially from A to Z.
- **Behavior**: Iterates over the 26 characters of the string and calculates the `charToIndex` for each, storing it in the `wiring` array.
- **Validation**: 
  - Asserts that `wiringConfig.size() == 26`.
  - Asserts that no letter maps to itself (`out != i`). A core cryptographic weakness of the historical Enigma was that a letter could never encrypt to itself, which the reflector enforces mechanically.

### `int reflect(int signal) const`
- **Description**: Passes an electrical signal through the reflector's wiring.
- **Parameters**:
  - `signal` (int): The 0-indexed integer signal entering the reflector from the leftmost rotor.
- **Returns**: The new 0-indexed integer signal being bounced back into the rotors.
- **Validation**: Asserts that the input signal is within the valid `[0, 25]` bounds.
