/**
 * @page rotor_guide Rotor (Walze)
 * @brief Detailed documentation for rotor scrambling, ringstellung, and notch advancement.
 *
 * Rotors provide the core polyalphabetic substitution of the Enigma machine.
 *
 * ```mermaid
 * graph LR
 *     PinIn[Input Contact] --> ShiftIn[Apply Core Position & Ringstellung]
 *     ShiftIn --> CrossWire[Internal Cross-Wiring]
 *     CrossWire --> ShiftOut[Subtract Core Position & Ringstellung]
 *     ShiftOut --> PinOut[Output Contact]
 * ```
 *
 * ## Historical Military Rotors (I, II, III)
 *
 * | Rotor | Wiring Layout | Turnover Notch |
 * |---|---|---|
 * | **I** | `EKMFLGDQVZNTOWYHXUSPAIBRCJ` | **Q** (at R step) |
 * | **II** | `AJDKSIRUXBLHWTMCQGZNPYFVOE` | **E** (at F step) |
 * | **III** | `BDFHJLCPRTXVZNYEIWGAKMUSQO` | **V** (at W step) |
 *
 * ## Class Reference
 * See Rotor for member documentation.
 */
