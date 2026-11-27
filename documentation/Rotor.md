# Rotors (Walzen) {#rotor_guide}

@tableofcontents

The Enigma machine scrambles letters using a series of rotating wired wheels called rotors (*Walzen*).
The Enigma Workbench supports the full historical complement of ten Wehrmacht and Kriegsmarine rotors:
rotors **I through V** (standard Heer/Luftwaffe), naval rotors **VI, VII, and VIII** (Kriegsmarine with dual notches),
and Greek rotors **Beta** (β) and **Gamma** (γ) (Kriegsmarine M4).

## Historical Wirings and Turnover Notches {#rotor_wirings}

The table below details all ten supported rotor wirings, their mechanical turnover windows, and stepping capabilities.

| Rotor | Wiring from contacts A–Z | Turnover window(s) | Stepping type | Introduced / Service |
| --- | --- | --- | --- | --- |
| **I** | `EKMFLGDQVZNTOWYHXUSPAIBRCJ` | **Q** | Single notch | 1930 (Wehrmacht Enigma I, M3, M4) |
| **II** | `AJDKSIRUXBLHWTMCQGZNPYFVOE` | **E** | Single notch | 1930 (Wehrmacht Enigma I, M3, M4) |
| **III** | `BDFHJLCPRTXVZNYEIWGAKMUSQO` | **V** | Single notch | 1930 (Wehrmacht Enigma I, M3, M4) |
| **IV** | `ESOVPZJAYQUIRHXLNFTGKDCMWB` | **J** | Single notch | Dec 1938 (M3, M4, Wehrmacht) |
| **V** | `VZBRGITYUPSDNHLXAWMJQOFECK` | **Z** | Single notch | Dec 1938 (M3, M4, Wehrmacht) |
| **VI** | `JPGVOUMFYQBENHZRDKASXLICTW` | **Z**, **M** | Dual notch | 1939 (Kriegsmarine M3 & M4 naval) |
| **VII** | `NZJHGRCXMYSWBOUFAIVLPEKQDT` | **Z**, **M** | Dual notch | 1939 (Kriegsmarine M3 & M4 naval) |
| **VIII** | `FKQHTLXOCBJSPDZRAMEWNIUYGV` | **Z**, **M** | Dual notch | 1939 (Kriegsmarine M3 & M4 naval) |
| **Beta** (β) | `LEYJVCNIXWPBQMDRTAKZGFUHOS` | None | Stationary (non-stepping) | Feb 1942 (Kriegsmarine M4 U-boat) |
| **Gamma** (γ) | `FSOKANUERHMBTIYCWLQPZXVGJD` | None | Stationary (non-stepping) | Spring 1943 (Kriegsmarine M4) |

[Historical wiring tables reference](https://enigmamuseum.com/rotwirg.htm)

## Rotor Categories and Operational Behavior {#rotor_categories}

### Standard Single-Notch Rotors (I–V)
Rotors I through V each contain a single mechanical turnover notch. The notch position is read
immediately before a keypress completes. When a rotor stands at its turnover window (e.g., **Q** for Rotor I),
the pawl drops into the index notch and advances both the rotor itself and the adjacent rotor to its left.
See @ref stepping_mechanism "Rotor stepping" for the complete mechanical stepping sequence.

### Kriegsmarine Dual-Notch Rotors (VI–VIII)
Naval rotors VI, VII, and VIII were introduced specifically for Kriegsmarine naval operations.
Each wheel features **two distinct turnover notches** located at letters **Z** and **M** (spaced roughly
180 degrees apart on the circumference). As a consequence, these rotors advance their left-hand neighbor
twice per full rotation, significantly shortening the period between carries and increasing cryptographic complexity.

### Kriegsmarine M4 Greek Rotors (Beta & Gamma)
Greek rotors *Beta* and *Gamma* were introduced on February 1, 1942, to deploy the 4-rotor Kriegsmarine
Enigma M4 across the Atlantic U-boat fleet (under key code *Triton* / Bletchley Park *Shark*).
- **Physical design**: The M4 chassis maintained the standard Enigma dimensions. To fit a fourth wheel, the reflector was replaced with a thin reflector (*Bruno* or *Caesar*), leaving room for a slim Greek rotor in the fourth slot.
- **Stationary operation**: The Enigma M4 stepping mechanism does not engage the fourth slot. As in the historical machine, Greek rotors **do not step mechanically** during typing (`isSteppable() == false`). They are positioned manually to a starting window and ring setting, acting as a fourth stationary permutation stage immediately adjacent to the thin reflector.

## Position and Ring Setting (Ringstellung) {#rotor_offset}

Every rotor maintains two independent spatial configurations:
1. **Rotational Position (Grundstellung)**: The visible letter displayed in the inspection window (values `0` to `25`, corresponding to `'A'` through `'Z'`).
2. **Ring Setting (Ringstellung)**: The internal offset between the wiring core and the outer alphabet ring containing the turnover notch (values `0` to `25`, displayed in the Workbench as **01** to **26**).

### Signal Transformation Math
When an electrical signal passes through a rotor, the contact shift is calculated as:
```text
effective contact = (signal + position - ringSetting + 26) % 26
```

1. **Forward Path (Right to Left)**: The signal enters from the right contact plate at `effective contact`, passes through `forwardWiring`, and the offset `(position - ringSetting)` is subtracted before exiting the left pin:
```text
exit = (forwardWiring[effective contact] - (position - ringSetting) + 26) % 26
```
2. **Reverse Path (Left to Right)**: On the return path from the reflector, the signal enters from the left at the shifted contact, passes through `backwardWiring` (precomputed inverse permutation), and the offset is subtracted before continuing rightward to the next wheel or plugboard.

See @ref Rotor "Rotor API Reference" for class member functions and implementation details.
