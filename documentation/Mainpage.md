/**
 * @mainpage Enigma Workbench Documentation
 *
 * Welcome to the technical documentation for **Enigma Workbench**, an interactive Enigma machine simulator built with **C++20** and **Raylib**.
 *
 * ```mermaid
 * graph LR
 *     subgraph Interface [Raylib Workbench UI]
 *         GUI[Interactive Lampboard, Tape & Controls]
 *         Viewer[Historical Museum Photo Modal]
 *     end
 *
 *     subgraph Core [EnigmaCore Simulation Engine]
 *         direction TB
 *         Pawls[Mechanical Stepping Pawls]
 *         PB[Steckerbrett Plugboard]
 *         Rotors[Rotors I, II, III]
 *         Ref[Reflector B]
 *     end
 *
 *     GUI <--> Core
 * ```
 *
 * ## Architecture & Technical Deep Dives
 * - @subpage architecture "Architecture Overview": High-level architecture, class hierarchy, and Raylib event loop design.
 * - @subpage signal_flow "Signal Flow & Cryptographic Path": Step-by-step telemetry through rotors, reflector, and plugboard.
 * - @subpage stepping_mechanism "Rotor Stepping & Double-Stepping": Mechanical pawls and the historical double-stepping anomaly.
 *
 * ## Component Hardware Guides
 * - @subpage charactermap_guide "Charactermap Module": Character-to-index signal mappings and conversions.
 * - @subpage plugboard_guide "Plugboard (Steckerbrett)": Lead pairing and symmetric signal swapping.
 * - @subpage rotor_guide "Rotor (Walze)": Internal cross-wiring, turnover notches, and Ringstellung.
 * - @subpage reflector_guide "Reflector (Umkehrwalze)": Fixed reflector B wiring and involution principles.
 * - @subpage enigmamachine_guide "Enigma Machine Chassis": Complete orchestrator and signal telemetry recorder.
 *
 * ## Workbench User Interface
 * - workbench::Session: Active typing session, replay engine, and message buffer.
 * - workbench::Layout: Dynamic responsive coordinate and bounding box calculations.
 * - workbench::handleInput: Keyboard shortcuts, typing events, and button dispatch.
 * - workbench::drawWorkbench: Immediate-mode Raylib drawing passes for the machine canvas.
 */
