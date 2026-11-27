# Workbench guide {#workbench_guide}

@tableofcontents

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
