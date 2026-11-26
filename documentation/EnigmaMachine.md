/**
 * @page enigmamachine_guide Enigma Machine Simulation
 * @brief Chassis integration and multi-rotor coordination.
 *
 * The `EnigmaMachine` orchestrates:
 * - Mechanical rotor advancing prior to electrical conduction.
 * - Sequential forward passing through Plugboard, Rotor III, Rotor II, Rotor I.
 * - Reflection through Reflector B.
 * - Reverse passing through Rotor I, Rotor II, Rotor III, and Plugboard.
 * - Capturing intermediate signal telemetry for real-time visualization.
 *
 * ```mermaid
 * stateDiagram-v2
 *     [*] --> Idle
 *     Idle --> Stepping: processChar(c)
 *     Stepping --> ForwardCircuit: pawls advance
 *     ForwardCircuit --> Reflection: right-to-left
 *     Reflection --> BackwardCircuit: bounce
 *     BackwardCircuit --> Output: left-to-right
 *     Output --> Idle: signal emitted
 * ```
 *
 * ## Class Reference
 * See EnigmaMachine for complete interface details.
 */
