# Workbench Guide {#workbench_guide}

@tableofcontents

The Enigma Workbench provides an authentic, fully interactive desktop simulation of the historical Enigma cipher machine.
Rendered in hardware-accelerated Vulkan 1.3, it combines physical accuracy, procedural mechanical audio synthesis,
and real-time signal visualization with historical WWII configurations and cryptanalysis tools.

## Physical Lampboard and Typewriter Keyboard {#lampboard_keyboard}

The Enigma Workbench faithfully reproduces the physical architecture and tactile response of the historical machine:

- **Dedicated 26-Bulb Lampboard (*Glühlampenfeld*)**:
  - Located directly above the typewriter keys, arranged in the historical German QWERTZ layout.
  - Multi-ring annular gradient optical bloom discs transition smoothly from a glowing `#FFAA33` incandescent filament core to a soft amber outer halo.
  - Encased in antique brass circular bezels with dark translucent amber glass lenses.
  - When a circuit completes on key depression, the output bulb illuminates with zero latency.

- **Physical Typewriter Keycaps (*Tastatur*)**:
  - Arranged in authentic circular keycaps with dual-layer metallic nickel/chrome outer rims and dark bakelite enamel faces.
  - Features authentic 3-pixel downward mechanical depression travel upon keystroke actuation.
  - Can be operated via physical keyboard input or by clicking keycaps directly with the mouse.

- **Procedural Mechanical Audio Engine**:
  - Employs zero-asset mathematical sound synthesis (44.1 kHz 16-bit mono) to generate physical acoustics in real time:
    - **Key Strikes**: Resonant steel-on-stop impacts (~36ms).
    - **Ratchet Clicks**: Spring-loaded pawl ratchet turnover clicks (~48ms).
    - **Plug Snaps**: Brass banana plug socket friction and spring clamp snaps (~28ms).
  - Controlled by the **Sound** button in the header toolbar (`Sound on` / `Sound off`), with safe automatic fallback in headless or non-ALSA environments.

## Historical WWII Tagesschlüssel Presets {#historical_presets}

Click the **Presets ▾** button in the header toolbar to immediately configure the machine to verified wartime daily key sheets (*Tagesschlüssel*):

| Preset Name | Historical Date | Configuration & Operation |
| --- | --- | --- |
| **Operation Barbarossa** | 22 June 1941 | Wehrmacht Heer frontline key at the launch of the Eastern Front (Rotors II-IV-V, Reflector B, start `W-X-C`, rings `02-21-12`, 10 plug pairs). |
| **U-552 Triton Shark** | 1 February 1942 | Kriegsmarine M4 4-rotor operational key in North Atlantic U-boat patrol (Greek Beta, Rotors II-IV-I, Thin Reflector Bruno, start `V-J-Z-E`, rings `01-20-11-04`, 10 plug pairs). |
| **North Atlantic Wolfpack** | 14 December 1943 | Kriegsmarine M4 4-rotor operational key with naval dual-notch Rotor VII (Greek Gamma, Rotors VII-V-III, Thin Reflector Bruno, start `N-A-V-Y`, rings `12-05-18-22`, 10 plug pairs). |
| **Normandy Invasion** | 6 June 1944 | German Army Group B command circuit during the Allied D-Day landings (Rotors III-I-II, Reflector B, start `D-A-Y`, rings `06-14-25`, 10 plug pairs). |
| **Scharnhorst Final** | 26 December 1943 | Battle of the North Cape naval battle group cipher key (Rotors III-VI-VIII with dual-notch Rotors VI and VIII, Reflector B, start `U-Z-K`, rings `01-08-13`, 10 plug pairs). |

Selecting any preset immediately loads the rotor complement, starting positions, ring settings, reflector, and plugboard leads, and replays the current message tape.

## Knurled Cylindrical Rotors and Settings {#change_rotor_count}

The rotor bank sits between the message tape and the lampboard:
- **Rotor Count**: Use **+** beside **Rotor settings** to add Rotor IV and Rotor V. Use **−** to remove the rightmost rotor.
- **Inspection Windows**: Each rotor displays its live window letter inside an antique inspection window framed by knurled thumbwheel serration grooves.
- **Rotor Settings Fields**:
  - **Live**: Read-only display of the current window letter after the latest keystroke.
  - **Start**: The initial window letter (from `'A'` to `'Z'`) from which encryption begins.
  - **Ring**: The internal wiring offset (*Ringstellung*), displayed from **01** to **26**.
- **Replay Guarantee**: Changing any start position, ring setting, or the rotor count immediately recalculates and replays the entire tape from the beginning.

## Catenary Draped Plugboard (Steckerbrett) {#connect_plugboard}

The plugboard sits below the rotor settings:
- **Catenary Cords**: Realistic cubic Bezier patch cords drape with gravity sag, drop shadows, braided fabric texture, and metallic socket collars.
- **Patching Controls**:
  - Click an unpatched socket, then click a target socket to connect a cord.
  - Click a selected socket again to cancel selection.
  - Click an existing connected socket to unplug its cord.
  - Click **Clear leads** to remove all connected patch cords.
  - Supports up to 10 patch cords (20 letters connected, 6 letters unplugged).

## Entering and Editing Messages {#enter_message}

- **Typing**: Type letters using your keyboard or click on-screen typewriter keys. The Workbench automatically capitalizes letters, ignores spaces and punctuation, and accepts up to 240 letters on the tape.
- **Tape Display**: Both the input plaintext tape and the output ciphertext tape format letters into standard 5-letter military groups.
- **Undo**: Press `Backspace` to delete the last entered letter. The machine rewinds the rotor positions, stepping history, and signal trace to the exact state after the previous letter.
- **Clipboard**:
  - **Paste** (`Ctrl+V`): Inserts text from the system clipboard, normalizing letters and skipping invalid characters.
  - **Copy ciphertext** (`Ctrl+C`): Copies the entire ciphertext output as an unbroken string to the clipboard.
- **Clear Tape**: Click **Clear** to wipe the tape and return the rotors to their configured starting positions.
- **Reset Machine**: Click **Reset machine** to restore the default 3-rotor Wehrmacht configuration (Rotors I–III at `A`, rings at `01`, and default leads `A–Z` and `B–Y`).

## Signal Path Inspection {#inspect_signal_path}

Directly underneath the keyboard and plugboard, the **Signal path** panel provides a full-width real-time trace of the electrical circuit for the most recent keypress:
- **Top Row (Forward Pass)**: Follows the electrical signal entering the plugboard, progressing through each rotor from right to left, and reaching the reflector.
- **Bottom Row (Reverse Pass)**: Follows the reflected signal returning through each rotor from left to right, traversing the plugboard, and illuminating the final lamp.

## Deciphering and Cryptanalysis {#decipher_message}

Click **Decipher** in the top bar to open the modal cryptanalysis workstation:
- Test suspected keys without modifying your live Workbench configuration.
- Launch the Turing-Welchman Bombe search engine with a known crib.
- Watch the 3-drum Bombe scrambler bank animate in real time and monitor search throughput in Megakeys/second.
- Transfer recovered keys back into the live Workbench with one click.
See @subpage bombe_guide "Decipher and Bombe Search Guide" for detailed instructions.

## Historical Museum Photo {#machine_photo}

Click **Machine photo** to view a high-resolution photograph of an authentic Enigma machine preserved at the Imperial War Museum in London.
Press `Esc`, click **Close**, or click outside the photo dialog to return to the Workbench.
