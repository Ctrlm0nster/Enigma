# Signal Path and Circuit Flow {#signal_flow}

@tableofcontents

The electrical circuit of an Enigma machine completes instantaneously once a key is depressed and the rotors have stepped.
The current flows from the battery or transformer through the plugboard, across each rotor in right-to-left order,
reflects off the reflector, travels back through the rotors in left-to-right order, passes through the plugboard a second time,
and finally illuminates a lamp on the lampboard.

The Enigma Workbench visualizes this journey in the **Signal path** panel directly below the keyboard and controls, exposing
every intermediate letter stage.

## Circuit Flow Diagrams {#signal_diagrams}

### Standard 3-Rotor Wehrmacht Enigma (M3)

```mermaid
sequenceDiagram
    autonumber
    actor Key as Typewriter Key
    participant P1 as Steckerbrett (In)
    participant R3 as Rotor Right (Fast)
    participant R2 as Rotor Middle
    participant R1 as Rotor Left (Slow)
    participant UKW as Reflector (B / C)
    participant P2 as Steckerbrett (Out)
    actor Lamp as Lampboard Bulb

    Key->>P1: Depress key (e.g. 'A')
    Note over P1: Swaps letter if plugged
    P1->>R3: Forward pin entrance
    R3->>R2: Forward contact transformation
    R2->>R1: Forward contact transformation
    R1->>UKW: Enters reflector contact
    Note over UKW: Reflects via 13 reciprocal pairs
    UKW->>R1: Reverse pin entrance
    R1->>R2: Reverse contact transformation
    R2->>R3: Reverse contact transformation
    R3->>P2: Exits rotor bank to plugboard
    Note over P2: Swaps letter if plugged
    P2->>Lamp: Circuit completes & bulb glows
```

### Kriegsmarine 4-Rotor Enigma M4

In the Kriegsmarine M4, a non-stepping Greek rotor (*Beta* or *Gamma*) and a thin reflector (*Bruno* or *Caesar*)
occupy the leftmost position:

```mermaid
sequenceDiagram
    autonumber
    actor Key as Typewriter Key
    participant P as Plugboard
    participant R3 as Fast Rotor
    participant R2 as Middle Rotor
    participant R1 as Left Rotor
    participant Greek as Greek Rotor (Beta/Gamma)
    participant UKW as Thin Reflector
    actor Lamp as Lampboard Bulb

    Key->>P: Keypress
    P->>R3: Plugboard forward swap
    R3->>R2: Forward pass
    R2->>R1: Forward pass
    R1->>Greek: Forward pass into stationary Greek rotor
    Greek->>UKW: Enters thin reflector
    Note over UKW: Involutory reflection
    UKW->>Greek: Reverse pass through Greek rotor
    Greek->>R1: Reverse pass through rotor stack
    R1->>R2: Reverse pass
    R2->>R3: Reverse pass
    R3->>P: Plugboard reverse swap
    P->>Lamp: Output lamp illuminates
```

## Reading the Signal Trace {#signal_trace_fields}

Every keypress processed by @ref EnigmaMachine::processCharWithTrace generates a @ref SignalTrace
recording each intermediate contact.

| Field in @ref SignalTrace | Stage description | Visual representation in Workbench |
| --- | --- | --- |
| `input` | The pressed keycap character | Highlighted typewriter keycap |
| `plugboardForward` | Character after initial plugboard swap | Plugboard cable input socket |
| `rotorForward` | Vector of contact letters traversing rotors right-to-left | Top row of the Signal Path display |
| `reflected` | Reflected contact exiting the reflector | Leftmost turnaround point in the signal path |
| `rotorBackward` | Vector of contact letters traversing rotors left-to-right | Bottom row of the Signal Path display |
| `output` | Final character exiting the second plugboard pass | Glowing optical bloom bulb on the lampboard |

## Mathematical Formulation {#signal_math}

Let $P$ denote the plugboard permutation ($P = P^{-1}$).
Let $R_i$ denote the permutation of the $i$-th rotor given its rotational position $p_i$ and ring setting $r_i$.
Let $U$ denote the reflector permutation ($U = U^{-1}$).

For an $N$-rotor configuration ($i = 1$ rightmost fast rotor, $i = N$ leftmost rotor):
```text
E_K = P * R_1^-1 * R_2^-1 * ... * R_N^-1 * U * R_N * ... * R_2 * R_1 * P
```

Because $P = P^{-1}$ and $U = U^{-1}$, the entire composite mapping $E_K$ is symmetric and involutory:
```text
E_K^-1 = E_K
```
This mathematical identity guarantees that ciphering is completely reversible when the same machine settings $K$ are applied.
