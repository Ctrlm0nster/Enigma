# Cipher Core {#enigmamachine_guide}

@tableofcontents

The `EnigmaCore` library implements the historical Enigma cryptographic algorithms in modern, standard C++20.
It is decoupled from the GUI and rendering systems and can be linked independently into CLI utilities or test harnesses.

## Key Classes and Construction {#core_classes}

An Enigma machine configuration consists of three primary components:
1. @ref Plugboard "Plugboard": Involutory lead connections (*Steckerbrett*).
2. A sequence of @ref Rotor "Rotor" instances: Rotors ordered from left to right (from slow/stator rotor to fast rotor).
3. A @ref Reflector "Reflector": Involutory reflector disc (*Umkehrwalze*).

### Manual Construction Example
```cpp
#include "Charactermap.hpp"
#include "EnigmaMachine.hpp"
#include "Plugboard.hpp"
#include "Reflector.hpp"
#include "Rotor.hpp"

// Setup Plugboard
Plugboard plugboard;
plugboard.addWire(charToIndex('A'), charToIndex('Z'));
plugboard.addWire(charToIndex('B'), charToIndex('Y'));

// Setup Rotors (Left to Right: I, II, III)
// Rotor(wiring, notchChar, ringSetting)
Rotor rotor1("EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q', 0); // Rotor I, Ring 01
Rotor rotor2("AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E', 0); // Rotor II, Ring 01
Rotor rotor3("BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V', 0); // Rotor III, Ring 01

rotor1.setPosition(charToIndex('M'));
rotor2.setPosition(charToIndex('C'));
rotor3.setPosition(charToIndex('K'));

std::vector<Rotor> rotors{rotor1, rotor2, rotor3};

// Setup Reflector B
Reflector reflector("YRUHQSLDPXNGOKMIEBFZCWVJAT");

// Assemble machine
EnigmaMachine machine(plugboard, rotors, reflector);
```

### Factory Construction with Presets
For production use, `EnigmaSettings.hpp` provides the `enigma::makeMachine` factory function:
```cpp
#include "EnigmaSettings.hpp"

enigma::MachineSettings settings;
settings.rotorCount = 3;
settings.rotorTypes = {0, 1, 2, 0, 0}; // I, II, III
settings.starts = {charToIndex('A'), charToIndex('A'), charToIndex('A'), 0, 0};
settings.rings = {0, 0, 0, 0, 0};
settings.reflectorIndex = 1; // Reflector B

EnigmaMachine machine = enigma::makeMachine(settings);
```

## Processing Input {#machine_process_input}

| Method | Behavior and Return |
| --- | --- |
| @ref EnigmaMachine::processChar "processChar(char)" | Steps the rotors, encrypts an uppercase letter `A`–`Z`, and returns the encrypted character. Leaves non-letter bytes intact without advancing the rotors. |
| @ref EnigmaMachine::processCharWithTrace "processCharWithTrace(char)" | Steps the rotors, enciphers the letter, and returns a detailed @ref SignalTrace capturing all intermediate contact values. |
| @ref EnigmaMachine::processString "processString(string_view)" | Sequentially enciphers a sequence of characters, mutating the machine state across the message. |

### Character Normalization
The core processes uppercase letters `A`–`Z`. The helper function `enigma::normalizeLetter` in `EnigmaSettings.hpp` converts lowercase letters to uppercase and converts non-alphabetic characters to `'\0'`.

## Kriegsmarine Enigma M4 Support {#machine_m4}

To construct a Kriegsmarine Enigma M4 machine:
1. Select a thin reflector: `Thin B` (*Bruno*, index 3) or `Thin C` (*Caesar*, index 4).
2. Set the 4th rotor to a Greek rotor: `Beta` (index 8) or `Gamma` (index 9).
3. The Greek rotor is configured with `canStep = false`, ensuring it remains stationary during operation.
4. Set the remaining three slots to naval rotors (e.g. Rotors VI, VII, or VIII with dual turnover notches).

```cpp
enigma::MachineSettings m4Settings;
m4Settings.rotorCount = 4;
m4Settings.rotorTypes = {8, 1, 3, 0, 0}; // Beta, II, IV, I
m4Settings.reflectorIndex = 3;            // Thin B
m4Settings.starts = {charToIndex('V'), charToIndex('J'), charToIndex('Z'), charToIndex('E'), 0};
m4Settings.rings = {0, 19, 10, 3, 0};

EnigmaMachine m4 = enigma::makeMachine(m4Settings);
```

## Historical Codebook Presets {#machine_presets}

The function `enigma::historicalPresets()` returns a static span of verified wartime daily key settings (*Tagesschlüssel*):
- **Operation Barbarossa (1941)**: Wehrmacht Heer frontline key (Rotors II-IV-V)
- **U-552 Triton Shark (1942)**: Kriegsmarine M4 4-rotor U-boat key (Beta-II-IV-I)
- **North Atlantic Wolfpack (1943)**: Kriegsmarine M4 naval key (Gamma-VII-V-III with dual-notch Rotor VII)
- **Normandy Invasion (1944)**: Western Front key (Rotors III-I-II)
- **Scharnhorst Final (1943)**: Arctic battleship key (Rotors III-VI-VIII with dual-notch Rotors VI and VIII)

See @ref historical_presets "Historical Presets in Workbench Guide" for further operational context.
