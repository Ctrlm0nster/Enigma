/**
 * @page stepping_mechanism Rotor Stepping & The Double-Stepping Anomaly
 * @brief Mechanics of pawls, ratchet wheels, and the historical double-stepping anomaly.
 *
 * ## Mechanical Stepping Principles
 *
 * In the Enigma I machine (three-rotor military model):
 * - Rotors are numbered from left to right: Rotor 0 (slow), Rotor 1 (middle), Rotor 2 (fast).
 * - A set of three mechanical pawls levers forward on *every* single keypress.
 * - The rightmost rotor (Rotor 2) has ratchet teeth directly exposed to the right pawl, so it **always advances by 1 step** on every keystroke.
 * - The middle and left rotors only step when the pawl drops into the turnover notch of the rotor to its right.
 *
 * ```mermaid
 * stateDiagram-v2
 *     [*] --> CheckNotches
 *     CheckNotches --> RightSteps: Always on keystroke
 *     CheckNotches --> MiddleSteps: If Right is at notch OR Middle is at notch
 *     CheckNotches --> LeftSteps: If Middle is at notch
 *     RightSteps --> [*]
 *     MiddleSteps --> [*]
 *     LeftSteps --> [*]
 * ```
 *
 * ## The Double-Stepping Anomaly
 *
 * Due to the historical mechanical design of the three pawls swinging together on a single lever:
 * 1. When Rotor 2 reaches its notch, the middle pawl drops into the notch, causing Rotor 1 to step on the next press.
 * 2. When Rotor 1 is at *its own notch*, its ratchet tooth is engaged by the left pawl to step Rotor 0.
 * 3. Crucially, the middle pawl also engages Rotor 1's own ratchet wheel again, causing **Rotor 1 to step twice in consecutive keystrokes**!
 *
 * This quirk causes the middle rotor period to be \f$25 \times 26 = 650\f$ steps instead of \f$26 \times 26 = 676\f$ steps.
 *
 * ```mermaid
 * flowchart TD
 *     Start([Keypress]) --> StepCheck{Check Rotor Positions}
 *     StepCheck -->|Fast Rotor| S2[Rotor 2 Steps]
 *     StepCheck -->|Rotor 2 at Notch| S1A[Rotor 1 Steps]
 *     StepCheck -->|Rotor 1 at Notch| S1B[Rotor 1 Steps Again: Double-Stepping!]
 *     StepCheck -->|Rotor 1 at Notch| S0[Rotor 0 Steps]
 *     S2 --> End([Ready for Electrical Signal])
 *     S1A --> End
 *     S1B --> End
 *     S0 --> End
 * ```
 *
 * ## Implementation in EnigmaCore
 *
 * In `EnigmaMachine::stepRotors()`, this behavior is explicitly modeled:
 *
 * ```cpp
 * void EnigmaMachine::stepRotors() {
 *   const bool rightAtNotch = rotors[2].atNotch();
 *   const bool middleAtNotch = rotors[1].atNotch();
 *
 *   if (middleAtNotch) {
 *     rotors[0].step();
 *     rotors[1].step(); // Double stepping!
 *   } else if (rightAtNotch) {
 *     rotors[1].step();
 *   }
 *
 *   rotors[2].step(); // Fast rotor always steps
 * }
 * ```
 */
