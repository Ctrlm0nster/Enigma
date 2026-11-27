# Workbench guide {#workbench_guide}

@tableofcontents

## Physical Lampboard and Typewriter Keyboard {#lampboard_keyboard}

The Enigma Workbench faithfully reproduces the physical architecture of the historical machine:
- **Lampboard (*Glühlampenfeld*)**: An incandescent 26-bulb display sitting above the keyboard. When an electrical circuit closes, the corresponding amber bulb glows with warm incandescent bloom discs against a dark glass lens and brass bezel.
- **Typewriter (*Tastatur*)**: Circular keycaps styled with authentic chrome outer rims and dark bakelite enamel faces. Keystrokes exhibit physical downward depression travel.
- **Procedural Mechanical Audio**: Mathematical audio synthesis generates authentic mechanical key clacks, pawl ratchet rotor clicks, and plug snaps in real time. The audio engine can be muted or unmuted at any time via the **Sound** toggle in the top bar.

## Historical WWII Tagesschlüssel Presets {#historical_presets}

Use the **Presets ▾** button in the header toolbar to immediately configure the machine according to verified WWII wartime daily key sheets (*Tagesschlüssel*):
- **Operation Barbarossa (1941)**: Frontline Wehrmacht Heer key (Rotors II-IV-V)
- **U-552 Triton Shark (1942)**: 4-rotor Kriegsmarine M4 U-boat key (Beta-II-IV-I with Thin Reflector Bruno)
- **North Atlantic Wolfpack (1943)**: 4-rotor Kriegsmarine M4 key with dual-notch naval Rotors VII, VIII, VI and Greek Beta
- **Normandy Invasion (1944)**: Defense of the Atlantic Wall frontline key (Rotors V-II-IV)
- **Scharnhorst Final (1943)**: Arctic naval key during Operation Ostfront (Rotors III-VI-VIII)

## Enter and edit a message {#enter_message}

Type with the keyboard or click the on-screen keys. Use **Paste** or `Ctrl+V`
for longer text. The Workbench capitalizes ASCII letters, skips other
characters, and accepts up to 240 letters on the tape. Both readouts display
five-letter groups. **Copy ciphertext** and `Ctrl+C` copy the full output as
one unspaced string.

Use `Backspace` to remove the last letter and rewind the machine to the state
after the previous letter. **Clear** empties the tape and returns the rotors to
their configured starting positions. **Reset machine** also restores three
rotors, all starting positions at **A**, rings at **01**, and leads **A–Z**
and **B–Y**.

## Add rotors and set their positions {#change_rotor_count}

Use **+** beside **Rotor settings** to add Rotor IV, then Rotor V. Use **−**
to remove the rightmost rotor. The settings and signal path update together.
If you remove a rotor and add it back, its start and ring settings are restored.

| Field | Meaning |
| --- | --- |
| **Live** | Current window letter after the latest keypress |
| **Start** | Window letter from which the message begins |
| **Ring** | Wiring offset displayed from **01** through **26** |

Changing the count, a start position, or a ring replays the entire tape.
Four- and five-rotor setups extend the simulator's stepping rule with the
historical wirings of rotors IV and V. @ref rotor_guide "Rotor wirings and turnover positions"

## Connect the plugboard {#connect_plugboard}

Click an unused socket, then click another socket to connect the pair. Clicking
the first socket again cancels the selection. With no socket selected, click an
already connected socket to remove its lead. With a free socket selected,
clicking a connected socket replaces its existing lead. **Clear leads** removes
all pairs. The Workbench supports ten leads.

## Decipher a message {#decipher_message}

Open **Decipher** for a known key or a crib-based Bombe search. @subpage bombe_guide "Decipher and Bombe search"

1. Copy the ciphertext and record the rotor count, **Start** and **Ring**
   values for each rotor, and the plugboard pairs.
2. Set up the machine with those settings. If you are using the same session,
   select **Clear** to rewind it while preserving the settings.
3. Paste the ciphertext. The original letters appear on the ciphertext tape.

## View the original machine {#machine_photo}

Select **Machine photo** to see the Enigma photographed at the Imperial War
Museum. Press `Esc`, select **Close**, or click outside the photo panel to
return to the Workbench.
