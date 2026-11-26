# Plugboard Documentation

The `Plugboard` (or Steckerbrett) was a component located at the front of the historical Enigma machine. It allowed the operator to swap pairs of letters before the signal entered the rotors, and again after it exited the rotors, adding significant cryptographic complexity.

## Class Overview
The `Plugboard` class maintains a `std::array<int, 26>` called `wiring` which stores the mapping of each character index.

## Methods

### `Plugboard()`
- **Description**: Default constructor.
- **Behavior**: Initializes the plugboard in a "blank" state. It uses `std::iota` to populate the `wiring` array with an identity mapping (`0 -> 0`, `1 -> 1`, ..., `25 -> 25`). In this state, a letter passes through the plugboard unchanged.

### `void addWire(int a, int b)`
- **Description**: Connects a physical wire between two letter indices on the plugboard.
- **Parameters**:
  - `a` (int): The index of the first letter.
  - `b` (int): The index of the second letter.
- **Behavior**: Updates the internal wiring array symmetrically such that `a` maps to `b` and `b` maps to `a`.
- **Validation**: 
  - Asserts that `a != b` (a letter cannot be wired to itself).
  - Asserts that both `a` and `b` are completely unwired prior to this function call (i.e., they map to themselves). The Enigma hardware did not allow a single letter plug to be connected to multiple wires.

### `int swap(int input) const`
- **Description**: Processes an electrical signal passing through the plugboard.
- **Parameters**:
  - `input` (int): The 0-indexed integer signal entering the plugboard.
- **Returns**: The 0-indexed integer signal exiting the plugboard. If the letter is wired, it returns the connected pair. If it is unwired, it returns the original input.
- **Validation**: Asserts that the input is strictly between `0` and `25`.
