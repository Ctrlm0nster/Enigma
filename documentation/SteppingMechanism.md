# Rotor Stepping Mechanism {#stepping_mechanism}

@tableofcontents

The Enigma machine employs a mechanical odometer-like stepping mechanism driven by mechanical pawls and notched rings.
The stepping takes place **before** the electrical signal travels from the key to the lamp.

## The Stepping Sequence {#step_rule}

When a key is depressed on the typewriter keyboard:
1. **Rightmost Rotor Step**: The rightmost (fast) rotor always advances one position on every keypress.
2. **Turnover Notch Check**: The machine checks whether any rotor (other than the leftmost stationary rotor) currently displays its turnover notch in the active window:
   - If a rotor is at its notch position, both that rotor and the rotor to its immediate left are engaged by the mechanical pawl arms to advance on the stroke.
3. **Advance**: All engaged rotors step one position forward (e.g., from `A` to `B`, wrapping from `Z` to `A`).
4. **Electrical Circuit**: After mechanical movement finishes, the electrical circuit completes and the signal traverses the newly aligned contacts to illuminate a lamp.

## Dual-Notch Turnover (Rotors VI, VII, VIII) {#dual_notch_stepping}

While standard Wehrmacht rotors (I–V) have a single turnover notch, Kriegsmarine naval rotors **VI, VII, and VIII** feature **two turnover notches** at letters **Z** and **M**:
- When Rotor VI, VII, or VIII is in the right or middle position, it initiates a carry when stepping off **Z** (advancing to **A**) AND when stepping off **M** (advancing to **N**).
- This doubles the stepping frequency of the adjacent rotor to the left, causing the rotor bank to cycle through permutations twice as quickly.

## Kriegsmarine M4 Greek Rotor Mechanics {#greek_rotor_stepping}

In the 4-rotor Kriegsmarine Enigma M4:
- The fourth rotor position is occupied by a Greek rotor (*Beta* or *Gamma*).
- Historically, the M4 mechanism was constructed by modifying an Enigma I/M3 chassis: the pawl drive arm only spanned the first three moving rotors.
- Consequently, Greek rotors **do not step mechanically** during typing (`isSteppable() == false`). The Greek wheel stays stationary at its set position, acting as an adjustable stator between the thin reflector and the three stepping rotors.

## The Double-Stepping Anomaly {#double_step}

Because of how the mechanical pawls engage the ratchet teeth and notches, Enigma machines exhibit an anomaly known as the **double step**.

### Mechanical Explanation
In a mechanical Enigma:
- A single ratchet bar holds three spring-loaded pawls moving in unison on each keystroke.
- The rightmost rotor has no ring covering its ratchet teeth, so its pawl engages on every press.
- For the middle and left rotors, each pawl rides on the smooth circumference of the rotor ring to its right, kept away from its ratchet teeth until a notch rotates into place.
- When the middle rotor is at its turnover notch, its own pawl drops into its ratchet teeth while also allowing the pawl for the left rotor to engage.
- On the next keypress, the middle rotor moves forward **again** (stepped by its own pawl) while simultaneously pushing the left rotor forward!

### Step-by-Step Example (Rotors I–II–III)
Set rotors I, II, III to starting positions **A D V** (left to right):
- Rotor III's notch is at **V**.
- Rotor II's notch is at **E**.

| Press | Window positions | Moving rotors | Mechanical reason |
| --- | --- | --- | --- |
| **Start** | **A D V** | — | Initial configuration before keypress |
| **1st** | **A E W** | II, III | Rotor III was at notch **V**; advances itself and carries Rotor II from **D** to **E**. |
| **2nd** | **B F X** | I, II, III | Rotor II is now at its notch **E**! Its pawl drops into its own ratchet (stepping II to **F**) and pushes Rotor I (stepping I to **B**). Rotor III steps as usual to **X**. |
| **3rd** | **B F Y** | III | Neither Rotor II nor III is at a notch; only fast Rotor III steps. |

Notice that Rotor II stepped on **both the first and second keypresses** (the double step). This means the middle rotor's period is not 26 presses, but 25 presses during this cycle.

In the Enigma Workbench, you can observe this live: set starting positions to **A D V**, type two characters, and watch the **Live** indicator update in real time. @ref rotor_guide "Turnover positions for all rotors"
