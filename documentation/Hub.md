# Documentation hub {#docs_hub}

@tableofcontents

## Use the Workbench {#hub_use}

- @ref quick_start "Quick start" builds and opens the application with Mise.
- @subpage workbench_guide "Workbench guide" covers message entry, adding and removing rotors, plugboard leads, and deciphering a message.

## Understand the machine {#hub_machine}

- @subpage signal_flow "Signal path" follows a letter from the keyboard to the lamp.
- @subpage stepping_mechanism "Rotor stepping" shows when each rotor moves and walks through a double step.
- @subpage rotor_guide "Rotors" lists the wirings and turnover positions for I–V.
- @subpage plugboard_guide "Plugboard" explains the letter swaps at each end of the signal path.
- @subpage reflector_guide "Reflector B" describes the fixed return wiring.

## Work on the code {#hub_development}

- @subpage architecture "Architecture" maps the C++ cipher core and the Vulkan workbench.
- @subpage enigmamachine_guide "Cipher core" explains how to process letters and read the trace from C++.
- @subpage charactermap_guide "Character mapping" covers the internal signal representation.
- @ref workbench::Session "Session API" covers the message tape, settings, and live machine state.
