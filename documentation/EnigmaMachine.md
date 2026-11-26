# Cipher core {#enigmamachine_guide}

@tableofcontents

Construct an @ref EnigmaMachine with a @ref Plugboard, a vector of rotors
ordered from left to right, and a @ref Reflector. Set the starting position
and ring offset on each rotor before passing it to the machine.

## Process input {#machine_process_input}

| Method | Result |
| --- | --- |
| @ref EnigmaMachine::processChar "processChar" | Steps and returns the output of one uppercase ASCII letter. |
| @ref EnigmaMachine::processCharWithTrace "processCharWithTrace" | Steps and records every stage of the signal, including the output. |
| @ref EnigmaMachine::processString "processString" | Processes a string in order, preserving the machine state between letters and subsequent calls. |

The core passes bytes outside `A`–`Z` through unchanged and leaves the rotor
positions intact for those bytes. The Workbench converts lowercase letters
to uppercase and discards other input before passing it to the core.

## Reproduce a message {#machine_reproduce}

Create a fresh machine with matching rotors, start and ring settings,
reflector, and plugboard before processing the same input again or
deciphering its output. The Workbench rebuilds the machine this way during
undo and when settings change. @ref architecture "Architecture"
