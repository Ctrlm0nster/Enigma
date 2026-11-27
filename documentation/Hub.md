# Documentation Hub {#docs_hub}

@tableofcontents

Welcome to the Enigma Workbench technical documentation and C++ API reference.

## Operating the Workbench {#hub_use}

- @ref quick_start "Quick start": Build and launch the application using Mise and Vulkan 1.3.
- @subpage workbench_guide "Workbench guide": Comprehensive guide to the physical lampboard, typewriter keycaps, procedural audio engine, WWII Tagesschlüssel presets, knurled rotors, plugboard patching, and message editing.
- @subpage bombe_guide "Decipher and Bombe search": Decipher messages with known keys, perform Turing-Welchman Bombe constraint searches with cribs, monitor live scrambler drums, and evaluate cryptanalysis throughput.

## Understanding the Machine {#hub_machine}

- @subpage signal_flow "Signal path and circuit flow": Complete electrical sequence from typewriter key through plugboard, rotors, reflector, and lampboard.
- @subpage stepping_mechanism "Rotor stepping mechanism": Mechanical pawl-and-ratchet stepping, dual-notch naval turnovers, non-stepping Greek rotors, and the double-stepping anomaly.
- @subpage rotor_guide "Rotors (Walzen)": Historical wirings and turnover notches for Rotors I through VIII and Greek rotors Beta and Gamma.
- @subpage reflector_guide "Reflectors (Umkehrwalzen)": Historical wirings for Reflectors A, B, C, Thin Bruno, and Thin Caesar, and the mathematical properties of involutory reflection.
- @subpage plugboard_guide "Plugboard (Steckerbrett)": Physical catenary patch cords, socket manipulation, and cryptographic permutation space.

## Architecture and C++ Development {#hub_development}

- @subpage architecture "Architecture overview": Modular structure of `EnigmaCore`, the Bombe solver, procedural audio engine, and Vulkan 1.3 dynamic rendering pipeline.
- @subpage enigmamachine_guide "Cipher core API": Instantiating `EnigmaMachine`, running signal traces, using factory presets, and modeling Kriegsmarine M4.
- @subpage charactermap_guide "Character mapping": Internal 0-based contact indexing and character normalization utilities.
- @ref enigma::MachineSettings "MachineSettings struct": Data structure describing full Enigma configurations.
- @ref workbench::Session "workbench::Session": Application session managing live message state, rotor configuration, and replay history.
- @ref workbench::AudioEngine "workbench::AudioEngine": Procedural mathematical audio synthesis engine.
