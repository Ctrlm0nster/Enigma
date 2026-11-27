# Bug and security notes

Enigma Workbench simulates a historical cipher for education and testing.
Use modern cryptography to protect sensitive messages.

## Fixed issues

| Issue | Impact and fix |
| --- | --- |
| Pasting from the same session | A paste could read freed memory if the source was the session's own plaintext or ciphertext. The session now collects the accepted letters before changing either tape. |
| Invalid rotor or reflector wiring | Repeated rotor contacts could leave the reverse wiring incomplete. A reflector with nonreciprocal pairs could break deciphering. Both constructors now reject invalid wiring. |
| Startup crash on unavailable display | On Linux, Raylib 5.0 crashed at startup if a graphics display was unavailable. With the Vulkan 1.3 and GLFW architecture, GLFW initialization and display availability are verified first; failures report `Enigma Workbench could not open a graphics window.` and exit cleanly with code `1`. |

## Verify the fixes

Run the release tests, sanitizer tests, and documentation build from the
repository root.

```sh
mise run test
mise run sanitize
mise run docs
```

The regression tests cover pasting from both tapes and rejecting invalid
rotor and reflector wiring.

## Report an issue

For an ordinary bug, include the commit or version, operating system, steps
to reproduce it, and the expected and actual behavior. Include relevant log
excerpts after removing private data. [GitHub Issues](https://github.com/Ctrlm0nster/Enigma/issues)

For a security issue, request a private reporting channel before sharing
technical details or affected data.
