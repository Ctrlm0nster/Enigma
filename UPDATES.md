# Enigma Workbench updates

## Planned for the next release

The next release will focus on the path from encrypting a message to sharing a puzzle, finding the key in Bombe, and trying the recovered settings in the Workbench.

### Bombe and Decipher

- Clarify when **Known leads** can infer additional pairs and when **Known board** fixes the entire plugboard.
- Show the settings covered by a search and offer concrete ways to narrow it when it returns too many matches or when a search limit interrupts it.
- Create shareable challenges from the Workbench with ciphertext and selected hints. Keep the answer and machine key out of the shared challenge.
- Make recovered keys easier to compare and copy independently of the plaintext.
- Add a visible caret and selection to Decipher inputs, scrollable tapes, and clearer crib highlighting.
- Publish a worked example that shows how crib length, position, rotor scope, and plugboard mode affect the results.

### Desktop Workbench

- Improve the layout at the minimum window size, on ultrawide screens, and with display scaling.
- Add visible keyboard focus and keyboard operation for machine controls, presets, and dialogs.
- Make rotor and reflector models directly selectable and explain what **Live**, **Start**, and **Ring** control.
- Restore the last session on restart, with a way to clear the saved session.
- Use consistent control states and clearer feedback after copying text, loading a preset, or applying a recovered key.

### Downloads and verification

- Publish a tagged release with the desktop app, CLI, bundled assets, and checksums for the supported download targets.
- Check a core-only build, a Vulkan frame, and a full Workbench-to-Bombe round trip before publishing.
- Put installation and a short demo near the top of the README and link to the detailed guides.
- Record existing clang-tidy diagnostics so CI can flag newly introduced correctness warnings.

Bombe guidance and the challenge workflow come first. Release packages will follow once the app and its bundled assets pass the clean-install checks.
