# Enigma Workbench

An interactive Enigma machine simulator built with C++20 and Raylib. The app
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

You need [Mise](https://mise.jdx.dev/getting-started.html), a C++20 compiler,
and the platform libraries required by Raylib. Mise installs the pinned CMake
and Ninja versions.

```bash
mise trust
mise install
mise run run
```

Use `mise run build` to build without launching the app. Mise writes build
output to `build/mise/`.

CMake fetches Raylib 5.0 and Catch2 3.5.2 on first configuration. For an offline
build, place source copies of those versions in `libs/raylib/` and
`libs/Catch2/` before configuring.

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
- `src/main.cpp` runs the Raylib application. The files in `src/workbench/`
  handle the message and machine state, input, layout, and drawing.
- `test/test_main.cpp` contains the Catch2 test suite.

## License

Enigma Workbench is licensed under the [GNU General Public License v3.0
only](LICENSE) (`GPL-3.0-only`).
See [NOTICE](NOTICE) for the project's copyright notice.

The bundled dependencies and fonts retain their own licenses.

- [Raylib 5.0](https://github.com/raysan5/raylib/blob/5.0/LICENSE)
- [Catch2 3.5.2](https://github.com/catchorg/Catch2/blob/v3.5.2/LICENSE.txt)
- [Montserrat](assets/Montserrat-OFL.txt)
- [JetBrains Mono](assets/JetBrainsMono-OFL.txt)
- [Instrument Serif](assets/InstrumentSerif-OFL.txt)
- [Photograph and font credits](assets/ATTRIBUTION.md)
