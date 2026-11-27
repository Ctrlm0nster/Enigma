# Reflectors (Umkehrwalzen) {#reflector_guide}

@tableofcontents

The reflector (*Umkehrwalze* or UKW) is the fixed or settable mirror disc situated at the extreme
left of the rotor bank. It redirects incoming electrical signals from the leftmost rotor back through
the entire rotor stack in the reverse direction, returning ultimately to the plugboard and lampboard.

The Enigma Workbench supports all five primary historical reflectors:
- **Reflector A** (*Umkehrwalze A*)
- **Reflector B** (*Umkehrwalze B*)
- **Reflector C** (*Umkehrwalze C*)
- **Thin Reflector Bruno** (*Umkehrwalze B dünn*)
- **Thin Reflector Caesar** (*Umkehrwalze C dünn*)

## Historical Wirings {#reflector_wiring}

Each reflector contains 26 spring-loaded contact pins cross-wired internally into 13 non-intersecting, reciprocal pairs.

| Reflector | Wiring layout (A–Z) | Machine variant | Historical notes |
| --- | --- | --- | --- |
| **Reflector A** | `EJMZALYXVBWFCRQUONTSPIKHGD` | Wehrmacht Enigma I | Early standard pre-war reflector |
| **Reflector B** | `YRUHQSLDPXNGOKMIEBFZCWVJAT` | Wehrmacht Enigma I, M3 | Most widely deployed standard wartime reflector |
| **Reflector C** | `FVPJIAOYEDRZXWGCTKUQSBNMHL` | Wehrmacht Enigma I, M3 | Alternative standard wartime reflector |
| **Thin B (Bruno)** | `ENKQAUYWJICOPBLMDXZVFTHRGS` | Kriegsmarine M4 | Narrow reflector paired with Greek rotor Beta/Gamma |
| **Thin C (Caesar)** | `RDOBJNTKVEHMLFCWZAXGYIPSUQ` | Kriegsmarine M4 | Alternative narrow reflector for Enigma M4 |

### Standard Reflector B Contact Pairs
For example, standard Reflector B pairs contacts as follows:

```text
Input:   A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
Output:  Y R U H Q S L D P X N G O K M I E B F Z C W V J A T
```

A signal entering on contact `A` emerges on `Y`, and a signal entering on `Y` emerges on `A`.

## Kriegsmarine M4 Thin Reflectors {#thin_reflectors}

In the 4-rotor Kriegsmarine Enigma M4, the machine's external chassis had to accommodate four rotors
instead of three. To achieve this without altering the overall casing size:
1. The standard reflector was halved in thickness, producing the "thin" reflectors (*Bruno* and *Caesar*).
2. The freed space was occupied by a special non-stepping fourth rotor (*Beta* or *Gamma*).
3. Thin reflectors cannot be used with standard rotors alone—they are mechanically and historically designed to mate with a stationary Greek rotor in slot 4.

In the Enigma Workbench, selecting an M4 preset (such as **U-552 Triton Shark** or **North Atlantic Wolfpack**)
automatically configures the thin reflector and the appropriate Greek rotor.

## Reciprocal Property and Cryptographic Weakness {#reflector_properties}

The reflector gives the Enigma machine several distinct operational properties:

1. **Involutory Symmetry (Self-Reciprocal)**:
   Because every wire connects two distinct pins symmetrically, enciphering and deciphering are mathematically identical operations. If key configuration $K$ transforms plaintext letter $P$ into ciphertext $C$, that identical configuration $K$ transforms $C$ back into $P$. No separate "decrypt" mechanism is required.

2. **No Letter Ever Enciphers to Itself**:
   Because each pin connects to a *different* pin (no internal self-loops), a reflector guarantees that an input letter can never map to itself (`E_K(x) != x`).

> [!IMPORTANT]
> The impossibility of self-encryption was the critical architectural flaw exploited by Marian Rejewski, Alan Turing, Gordon Welchman, and the codebreakers at Bletchley Park. In the Turing-Welchman Bombe, this rule allows immediate elimination of false crib alignments where any crib letter matches the corresponding ciphertext character.

See @ref Reflector "Reflector API Reference" for class member functions and construction.
