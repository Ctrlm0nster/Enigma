# Architecture Overview {#architecture}

High-level system design and component relationships in Enigma Workbench.

## System Overview

Enigma Workbench separates the cryptographic hardware simulation from the Raylib interactive interface:
- **EnigmaCore**: Pure C++20 cryptographic engine modeling mechanical hardware (rotors, reflector, plugboard, stepping pawls). Has zero GUI dependencies and can be compiled standalone or into headless test suites.
- **Workbench UI**: An interactive Raylib interface maintaining typing sessions, signal telemetry visualization, modal photo viewer, and responsive layouts.

```mermaid
classDiagram
    class EnigmaMachine {
        -Plugboard plugboard
        -vector~Rotor~ rotors
        -Reflector reflector
        -stepRotors() void
        +processChar(char c) char
        +processCharWithTrace(char c) SignalTrace
        +processString(string input) string
    }

    class Rotor {
        -array~int,26~ forwardWiring
        -array~int,26~ backwardWiring
        -int position
        -int notch
        -int ringSetting
        +step() void
        +atNotch() bool
        +forward(int signal) int
        +backward(int signal) int
    }

    class Plugboard {
        -array~int,26~ wiring
        +addWire(int a, int b) void
        +swap(int input) int
    }

    class Reflector {
        -array~int,26~ wiring
        +reflect(int signal) int
    }

    class Session {
        -array~int,3~ starts_
        -array~int,3~ rings_
        -vector~pair~ plugPairs_
        -EnigmaMachine machine_
        -string input_
        -string output_
        -optional~SignalTrace~ trace_
        +typeLetter(char letter) bool
        +paste(string_view text) size_t
        +backspace() void
        +adjustRotor(size_t rotor, int startDelta, int ringDelta) void
    }

    EnigmaMachine *-- Plugboard
    EnigmaMachine *-- Rotor
    EnigmaMachine *-- Reflector
    Session *-- EnigmaMachine
```

## Data and Event Flow

When a user types a letter or clicks the GUI keyboard:
1. `workbench::handleInput` catches the event.
2. `workbench::Session::typeLetter` appends the character to the session buffer.
3. The internal `EnigmaMachine::processCharWithTrace` steps the rotors, traces the electrical signal through every component, and returns the ciphertext letter along with a `SignalTrace`.
4. `workbench::drawWorkbench` renders the updated ciphertext tape, lights up the corresponding lamp on the lampboard, and draws the stage-by-stage path in the signal visualizer.

```mermaid
flowchart TD
    User([User Keystroke / Click]) --> Input[workbench::handleInput]
    Input --> Session[workbench::Session::typeLetter]
    Session --> Machine[EnigmaMachine::processCharWithTrace]
    subgraph Simulation [EnigmaCore Hardware]
        Machine --> Step[stepRotors: Advance Pawls]
        Step --> PB1[Plugboard Forward]
        PB1 --> RF[Rotors Forward III -> II -> I]
        RF --> Ref[Reflector B]
        Ref --> RB[Rotors Reverse I -> II -> III]
        RB --> PB2[Plugboard Return]
    end
    PB2 --> Trace[SignalTrace Telemetry]
    Trace --> View[workbench::drawWorkbench]
    View --> Screen([Rendered Raylib Canvas])
```
