# Enigma Workbench

An interactive Enigma machine simulator built with C++20 and Raylib. The app
uses the historical I–III rotor wirings, their turnover notches, Reflector B,
ring settings, double stepping, and a configurable plugboard.

## Interface

- Type with the physical keyboard or click the on-screen lampboard.
- See plaintext and ciphertext grouped into traditional five-letter blocks.
- Follow the real signal state through the plugboard, rotors, reflector, and
  return path for every keypress.
- Change each rotor's starting position and ring setting. Existing text is
  automatically re-encrypted with the new configuration.
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
build, place local source copies in `libs/raylib/` and `libs/Catch2/` before
configuring.

## Tests

```bash
mise run test
```

The tests cover encryption/decryption reversibility, signal tracing, and
replaying a message after undo or machine setting changes.

Use a separate CMake build directory for sanitizer builds.

```bash
mise exec -- cmake -G Ninja -S . -B build/sanitized -DCMAKE_BUILD_TYPE=Debug -DENIGMA_ENABLE_SANITIZERS=ON
mise exec -- cmake --build build/sanitized
```

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
