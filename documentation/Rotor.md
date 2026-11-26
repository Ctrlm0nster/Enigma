# Rotors {#rotor_guide}

@tableofcontents

## Wirings and turnover positions {#rotor_wirings}

The Workbench places the rotors in the order shown below, from left to right.
It starts with I–III and adds IV and V on the right.

| Rotor | Wiring from contacts A–Z | Turnover window |
| --- | --- | --- |
| I | `EKMFLGDQVZNTOWYHXUSPAIBRCJ` | Q |
| II | `AJDKSIRUXBLHWTMCQGZNPYFVOE` | E |
| III | `BDFHJLCPRTXVZNYEIWGAKMUSQO` | V |
| IV | `ESOVPZJAYQUIRHXLNFTGKDCMWB` | J |
| V | `VZBRGITYUPSDNHLXAWMJQOFECK` | Z |

[Historical wiring tables](https://enigmamuseum.com/rotwirg.htm)

The turnover window is read before the keypress. A rotor at that window
can advance itself and the rotor to its left. @ref stepping_mechanism "Rotor stepping"

## Position and ring offset {#rotor_offset}

The start setting determines the initial window letter. The ring setting
changes the alignment of the wiring relative to that letter. In the code,
both values use an index from `0` to `25`. The Workbench shows rings from
**01** to **26**.

For each signal, @ref Rotor adds the difference between the live position
and ring setting, looks up the shifted contact in the wiring, and subtracts
the offset from the result. Each calculation wraps around the 26 letters.
The return path uses the inverse wiring with the same offset.
