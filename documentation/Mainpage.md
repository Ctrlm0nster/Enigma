# Enigma Workbench

<span class="docs-lead">Run an Enigma machine on your desktop. Follow each keypress through the plugboard, rotors, reflector, and lamp, and see how changing the machine settings changes the message.</span>

<div class="docs-start" aria-label="Quick links">
@ref quick_start "Quick start"
@ref docs_hub "Documentation hub"
@ref workbench_guide "Workbench guide"
[GitHub](https://github.com/Ctrlm0nster/Enigma)
</div>

@htmlonly
<figure class="workbench-preview">
<img src="workbench.png" width="1320" height="840" alt="Enigma Workbench with five rotor settings, a two-row signal trace, a plugboard, and an amber output lamp" />
<figcaption>The Workbench with rotors I–V active. The signal path and settings grow as you add rotors.</figcaption>
</figure>
@endhtmlonly

## Quick start {#quick_start}

Install Mise, a C++20 compiler, and the system libraries required by Raylib.
From the repository directory, run the following commands. [Build prerequisites](https://github.com/Ctrlm0nster/Enigma#build-and-run)

```sh
mise trust
mise install
mise run run
```

Type `HELLO` or click the on-screen keys. The output appears on the ciphertext
tape, and the amber lamp shows the latest output letter. Use the **+** button
beside **Rotor settings** to add Rotor IV, then Rotor V. The app replays `HELLO`
from your chosen starting positions each time you change the rotor count.

## Controls at a glance {#quick_controls}

| Action | Control |
| --- | --- |
| Enter one letter | Keyboard or on-screen keys |
| Paste a message | **Paste** or `Ctrl+V` |
| Copy the output | **Copy ciphertext** or `Ctrl+C` |
| Undo the last letter | `Backspace` |
| Add or remove a rotor | **+** or **−** beside **Rotor settings** |
| Start a new message with the same settings | **Clear** |
| Restore the default configuration | **Reset machine** |

## More documentation {#more_documentation}

- @subpage docs_hub "Documentation hub" for operating instructions, the rotor model, and component guides.
- @ref architecture "Architecture" for the layout of the C++20 core and Raylib workbench.
- @ref EnigmaMachine "EnigmaMachine API" for the cipher core and its trace methods.

Enigma Workbench is released under GPL-3.0-only. [License](https://github.com/Ctrlm0nster/Enigma/blob/master/LICENSE) · [Photo and font credits](https://github.com/Ctrlm0nster/Enigma/blob/master/assets/ATTRIBUTION.md)
