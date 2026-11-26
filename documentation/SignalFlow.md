# Signal path {#signal_flow}

@tableofcontents

Press a key to see the route on the **Signal path** panel. The rotors
step first. The top row follows the signal from the key to Reflector B.
The bottom row follows it back through the machine to the lamp.

## Three-rotor example {#signal_example}

```mermaid
sequenceDiagram
    actor Key as Keyboard
    participant P as Plugboard
    participant III as Rotor III
    participant II as Rotor II
    participant I as Rotor I
    participant B as Reflector B
    actor Lamp

    Key->>P: Input letter
    P->>III: After the first swap
    III->>II: Forward contact
    II->>I: Forward contact
    I->>B: Forward contact
    B->>I: Reflected contact
    I->>II: Reverse contact
    II->>III: Reverse contact
    III->>P: Reverse contact
    P->>Lamp: Output letter
```

With four or five rotors, the signal enters the rightmost active rotor after
the plugboard and passes through every rotor on both legs of the trip.

## Read the trace {#signal_trace_fields}

| Field in @ref SignalTrace | What it records |
| --- | --- |
| `input` | The pressed letter |
| `plugboardForward` | The letter after the first plugboard swap |
| `rotorForward` | Output of each rotor, from right to left |
| `reflected` | Reflector B output |
| `rotorBackward` | Output of each rotor on the return path, from left to right |
| `output` | The last plugboard result, shown on the lamp and output tape |
