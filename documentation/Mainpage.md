# Enigma Workbench

<span class="docs-lead">An authentic, high-performance Enigma machine simulator and Turing-Welchman Bombe built with modern C++20 and custom Vulkan 1.3 rendering. Features a physical lampboard, tactile typewriter keycaps, draped cubic Bezier patch cords, zero-asset procedural mechanical audio synthesis, historical Kriegsmarine M4 modeling, and WWII Tagesschlüssel presets.</span>

<div class="docs-start" aria-label="Quick links">
@ref quick_start "Quick start"
@ref docs_hub "Documentation hub"
@ref workbench_guide "Workbench guide"
@ref bombe_guide "Bombe search"
[GitHub](https://github.com/Ctrlm0nster/Enigma)
</div>

@htmlonly
<figure class="workbench-preview">
<img src="workbench.png" width="1320" height="840" alt="Enigma Workbench with five rotors, a separate lampboard and keyboard, and a full-width signal path below the controls" />
<figcaption>The signal path follows the last keypress through five rotors to the amber output lamp.</figcaption>
</figure>
@endhtmlonly

## Quick start {#quick_start}

Install Mise, a C++20 compiler, a Vulkan 1.3-capable GPU driver, and the system Vulkan, GLFW, FreeType, and libjpeg-turbo development packages. Install `glslc` to compile shaders.
From the repository directory, run:

```sh
mise trust
mise install
mise run run
```

Type `HELLO` or click the on-screen keys. The output appears on the ciphertext
tape, and the amber lamp shows the latest output letter. Use the **Presets ▾** button
to load historical WWII keys, or use **+** beside **Rotor settings** to add fourth and fifth rotors.

## Controls at a glance {#quick_controls}

| Action | Control |
| --- | --- |
| Enter one letter | Keyboard or on-screen typewriter keys |
| Paste a message | **Paste** or `Ctrl+V` |
| Copy the ciphertext | **Copy ciphertext** or `Ctrl+C` |
| Undo the last letter | `Backspace` (rewinds machine and signal trace) |
| Toggle mechanical sound | **Sound** button (`Sound on` / `Sound off`) |
| Load WWII key sheets | **Presets ▾** dropdown menu |
| Add or remove a rotor | **+** or **−** beside **Rotor settings** |
| Adjust start / ring | Window letter dropdowns and ring numeric inputs |
| Connect / remove plug leads | Click unpatched or connected sockets (up to 10 pairs) |
| Clear plugboard | **Clear leads** |
| Start a new message | **Clear** |
| Restore default configuration | **Reset machine** |
| Open cryptanalysis suite | **Decipher** (known key decoding & Bombe search) |
| View original machine | **Machine photo** (Imperial War Museum photo) |

## Documentation Topics {#more_documentation}

- @subpage docs_hub "Documentation hub": Master index for operating guides, machine mechanics, and C++ reference.
- @subpage workbench_guide "Workbench guide": Full guide to the lampboard, typewriter keycaps, audio, presets, rotors, and plugboard.
- @subpage bombe_guide "Decipher and Bombe search": Deciphering intercepted text, crib searches, scrambler drum animations, and throughput telemetry.
- @subpage signal_flow "Signal path and circuit flow": Visualizing electrical current through the machine.
- @subpage stepping_mechanism "Rotor stepping mechanism": Mechanical pawl engagement, naval dual-notch turnover, and the double-stepping anomaly.
- @subpage rotor_guide "Rotors (Walzen)": Wirings, notch positions, and Greek rotor characteristics for all 10 rotors.
- @subpage reflector_guide "Reflectors (Umkehrwalzen)": Standard and thin reflectors (A, B, C, Thin Bruno, Thin Caesar).
- @subpage plugboard_guide "Plugboard (Steckerbrett)": Catenary Bezier cords and permutation math.
- @subpage architecture "Architecture overview": C++20 core, audio engine, Bombe solver, and Vulkan 1.3 renderer.
