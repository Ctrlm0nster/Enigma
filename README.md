# Enigma Workbench

An interactive Enigma machine simulator built with C++20 and Vulkan 1.3. The app
uses the historical I–V rotor wirings, their turnover notches, Reflector B,
ring settings, double stepping, and a configurable plugboard.

## Interface

- Type with the physical keyboard or click the on-screen keys. The output key
  lights up in amber.
- Paste a message with `Ctrl+V`. The machine accepts letters A–Z and ignores
  spaces and punctuation.
- See plaintext and ciphertext grouped into traditional five-letter blocks.
- Follow the real signal state through the plugboard, rotors, reflector, and
  return path for every keypress.
- Select **Machine photo** to inspect the original Enigma at the Imperial War
  Museum.
- Change each rotor's starting position and ring setting. Existing text is
  automatically re-encrypted with the new configuration.
- Use the **−** and **+** controls next to **Rotor settings** to change the
  number of active rotors from three to five. The signal diagram and settings
  update immediately. Four- and five-rotor configurations extend the simulator's
  stepping model with the historical wirings for Rotors IV and V.
- Click two plugboard letters to connect them; click an active letter to remove
  its lead. Up to ten leads are supported.
- Use Backspace to undo a keypress and accurately rewind the machine, or copy
  the ciphertext with the button or `Ctrl+C`.
- Resize the window down to 1080×720 without losing controls.

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
