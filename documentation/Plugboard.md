# Plugboard (Steckerbrett) {#plugboard_guide}

@tableofcontents

The plugboard (*Steckerbrett*) is the patch panel situated on the front of the Enigma machine.
It introduces an involutory substitution permutation before the electrical signal enters the rotor stack,
and swaps the letters a second time when the signal returns from the rotors toward the lampboard.

## Physical Catenary Draped Patch Cords {#plugboard_physical}

The Enigma Workbench models the plugboard with physical fidelity:
- **Catenary Draped Patch Cords**: Rendered as smooth cubic Bezier splines that sag naturally under gravity, casting soft drop shadows onto the socket plate below.
- **Braided Texture & Metallic Collars**: Each cord features dual-tone braided fabric jacket colors and knurled metallic socket contact collars.
- **Socket Contacts**: 26 dual-prong jacks labeled A through Z arranged in standard German keyboard layout.

## Operating the Plugboard {#plugboard_operation}

- **Connect a Lead**: Click any unoccupied socket to select it, then click a second socket to complete the lead.
- **Cancel Selection**: Clicking the selected socket a second time cancels the action.
- **Remove a Lead**: With no socket selected, click any connected socket to unplug its lead.
- **Swap a Lead**: With an unused socket selected, clicking an already-connected socket detaches the old connection and links the new socket.
- **Clear All Leads**: Click **Clear leads** to remove all connected patch cords.
- **Lead Limit**: The Workbench supports up to 10 patch cords (20 plugged letters), matching historical Wehrmacht operational protocol (which left 6 letters unplugged).

## Cryptographic Impact {#plugboard_crypto}

The plugboard dramatically increases the key space of the machine:
- With 10 cables connecting 20 letters out of 26, the number of possible plugboard wirings is:
  ```text
  26! / ((26 - 2 * 10)! * 10! * 2^10) = 26! / (6! * 10! * 1024) ≈ 1.507 × 10^14
  ```
- While the plugboard vastly expands the number of daily key combinations, it is a strictly static monoalphabetic substitution that does not step with keystrokes. As Marian Rejewski proved in 1932, the cycles of rotor permutations can be analyzed independently of plugboard connections—a mathematical principle that underpinned both the Polish *Bomba* and Turing's Bombe.

See @ref Plugboard "Plugboard API Reference" for internal array mapping and connection validation.
