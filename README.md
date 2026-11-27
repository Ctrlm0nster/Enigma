# Enigma Workbench

An authentic, high-performance Enigma machine simulator and Turing-Welchman Bombe
built with modern C++20 and custom Vulkan 1.3 rendering. Features a physical lampboard,
tactile typewriter keycaps, draped cubic Bezier patch cords, zero-asset procedural
mechanical audio synthesis, historical Kriegsmarine M4 modeling, and WWII Tagesschlüssel presets.

## Flagship Features

### 1. Authentic Visual & Physical Simulation
- **Dedicated 26-Bulb Lampboard (*Glühlampenfeld*)**: Multi-ring annular gradient optical bloom discs (`#FFAA33` filament core to soft outer halo) encased in antique brass bezels with dark translucent amber glass lenses.
- **Physical Typewriter Keycaps (*Tastatur*)**: Circular dual-layer metallic nickel/chrome rims with dark bakelite enamel faces and 3px downward mechanical travel upon keystroke depression.
- **Catenary Draped Plugboard Leads (*Steckerbrett*)**: Realistic cubic Bezier patch cords with gravity sag, ambient drop shadows, braided fabric texture jackets, and metallic socket collars.
- **Knurled Cylindrical Rotors**: Live inspection windows with knurled thumbwheel serration grooves and individual model designations.

### 2. Procedural Mechanical Audio Engine
- Zero-external-audio-asset procedural mathematical synthesis (44.1kHz mono):
  - Typewriter mechanical key strikes with resonant metal plink (~36ms)
  - Metallic pawl ratchet turnover and rotor detent clicks (~48ms)
  - Plugboard banana plug contact friction and socket spring clamp snaps (~28ms)
- Thread-safe non-blocking ALSA PCM streaming with sound toggle (`Sound on` / `Sound off`) and automatic headless/CI fallback.

### 3. Historical Kriegsmarine Enigma M4 & WWII Codebook Presets
- Full Kriegsmarine Enigma M4 support with stationary Greek rotors *Beta* ($\beta$) and *Gamma* ($\gamma$), Thin Reflectors *Bruno* and *Caesar*, and dual turnover notches on Rotors VI, VII, and VIII (turning over at both 'Z' and 'M').
- One-click authentic WWII *Tagesschlüssel* codebook loader:
  - **Operation Barbarossa (1941)**: Frontline Wehrmacht Heer key (Rotors II-IV-V)
  - **U-552 Triton Shark (1942)**: Kriegsmarine M4 4-rotor U-boat key (Beta-II-IV-I, Thin B)
  - **North Atlantic Wolfpack (1943)**: Kriegsmarine M4 wolfpack key (Beta-VII-VIII-VI, Thin B)
  - **Normandy Invasion (1944)**: Western Front key (Rotors V-II-IV)
  - **Scharnhorst Final Battle (1943)**: Arctic battleship key (Rotors III-VI-VIII)

### 4. Turing-Welchman Bombe Scrambler Bank & Benchmarking
- Real-time rotating cylindrical scrambler drum visualization (crimson, Prussian blue, and forest green dials) animating during search decipher.
- High-throughput key search benchmark reporting throughput in Megakeys/second.

## Build and run

Install a C++20 compiler, a Vulkan 1.3-capable driver, and development packages
for Vulkan, GLFW, FreeType, and libjpeg-turbo. Install `glslc` to compile the
shaders. Mise installs the pinned CMake and Ninja versions. [Mise installation](https://mise.jdx.dev/getting-started.html)

```bash
mise trust
mise install
mise run run
```

Use `mise run build` to build without launching the app. Mise writes build
output to `build/mise/`.

CMake fetches Catch2 3.5.2 for the tests. For an offline build, place a source
copy of Catch2 in `libs/Catch2/` before configuring.

Run `mise run configure` before opening the project with clangd. It generates
the compilation database that `.clangd` uses.

## Tests

```bash
mise run test
```

The tests cover encryption/decryption reversibility, signal tracing, and
replaying a message after undo or machine setting changes.

Use `mise run sanitize` to configure, build, and run the test suite with
AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
mise run sanitize
```

See [Bug and security notes](bug-security.md) for fixes and reporting details.

## Documentation

The [documentation site](https://ctrlm0nster.github.io/Enigma/) brings together
the Workbench guide, signal diagrams, architecture, and C++ API reference.
To build it locally, install Doxygen 1.18.0 and Graphviz, then run the following.

```bash
mise run docs
```

Open `docs/html/index.html` in a browser. The site uses
[Doxygen Awesome v2.5.0](https://github.com/jothepro/doxygen-awesome-css/tree/v2.5.0)
under its [MIT license](documentation/theme/LICENSE).

## Structure

- `EnigmaCore` contains character mapping, plugboard, rotor, reflector, stepping,
  encryption, and signal-trace logic.
- `src/main.cpp` runs the desktop application loop with GLFW and Vulkan 1.3.
  The files in `src/workbench/` handle message state, input events, layout,
  2D draw batching, and Vulkan GPU rendering.
- `test/test_main.cpp` contains the Catch2 test suite.

## License

Enigma Workbench is licensed under the [GNU General Public License v3.0
only](LICENSE) (`GPL-3.0-only`).
See [NOTICE](NOTICE) for the project's copyright notice.

The bundled dependencies and fonts retain their own licenses.

- [GLFW](https://www.glfw.org/license.html)
- [FreeType](https://freetype.org/FTL.TXT)
- [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo/blob/main/LICENSE.md)
- [Catch2 3.5.2](https://github.com/catchorg/Catch2/blob/v3.5.2/LICENSE.txt)
- [Montserrat](assets/Montserrat-OFL.txt)
- [JetBrains Mono](assets/JetBrainsMono-OFL.txt)
- [Instrument Serif](assets/InstrumentSerif-OFL.txt)
- [Photograph and font credits](assets/ATTRIBUTION.md)
