# Decipher and Turing-Welchman Bombe Search {#bombe_guide}

@tableofcontents

The Enigma Workbench includes a dedicated cryptanalysis suite accessible via the **Decipher** button in the header toolbar.
The Decipher workstation provides two modes of operation:
1. **Decipher with a known key**: Instantly decode intercepted ciphertext when machine settings are known or partially reconstructed.
2. **Bombe search with a crib** can recover a rotor order, reflector, starting windows, and plugboard leads.

The Decipher workstation operates in its own sandboxed state, allowing you to experiment with ciphertext and recovered keys without losing your live Workbench session.

## Decipher with a Known Key {#bombe_known}

When you have intercepted an enciphered transmission and know (or wish to test) specific key settings:

1. **Enter Ciphertext**: Paste the ciphertext into the **Ciphertext** field (supports up to 240 ASCII letters, ignoring spaces, punctuation, and converting lowercase letters).
2. **Configure Key Settings**:
   - **Start positions**: Enter the initial window letters (e.g., `AAA` for 3 rotors, `VJZE` for 4-rotor M4).
   - **Ring settings**: Enter two-digit ring numbers from **01** to **26** ordered left to right (e.g., `01 01 01` or `01 20 11 04`).
   - **Plugboard leads**: Enter paired letters (e.g., `AV BS CG DL FU HZ IN KM OW RX`).
3. **Use Workbench key** imports the current rotor models, starting windows, rings, reflector, and plugboard leads from the Workbench.
4. **Execute**: Select **Decipher**. The decrypted plaintext appears in the results preview panel.
5. **Export & Actions**:
   - Click **Copy plaintext** to copy the decrypted output to the system clipboard.

> [!TIP]
> For a quick known-key test: enter ciphertext `BDZGO`, start windows `AAA`, rings `01 01 01`, Reflector B, and an empty plugboard. Deciphering yields the plaintext `AAAAA`.

## Turing-Welchman Bombe Search with a Crib {#bombe_search}

Switch to the **Bombe search** tab. A *crib* is a suspected fragment of plaintext expected to appear somewhere within the ciphertext (for example, standard German weather report openings such as `WETTER` or formal closing headers).

Select **Try challenge** to generate a message with a hidden rotor order, starting windows, and reflector. The app fills in a crib, ring settings, and plugboard for the challenge. Each click generates a fresh key.

### 1. Configure the Search Parameters
- **Ciphertext & Crib**: Enter the intercepted ciphertext and the suspected plaintext crib.
- **Rings** gives the known ring settings in left-to-right slot order.
- **Crib position** takes the crib's first position in the ciphertext, counting letters from 1. Leave it blank to scan all offsets.

### 2. Crib Alignment and Non-Self-Encryption Filtering
Because the Enigma reflector can never connect a contact to itself, an Enigma machine can never encipher a letter to itself (`E_K(x) != x`).
The Bombe search engine exploits this rule to immediately discard non-viable crib offsets:
```text
If ciphertext[i + offset] == crib[i], then alignment is rejected
```
The search progress telemetry reports both total potential alignments and the count of surviving viable offsets.

### 3. Search Scopes
Select the search scope based on how much is known about the daily key sheet:
- **Rotor Scope**:
  - **I–III fixed**: Tests only the standard order (Rotors I-II-III).
  - **I–III orders**: Tests all 6 permutations of Rotors I, II, and III (3! = 6).
  - **I–V orders**: Tests all 60 permutations of five historical Wehrmacht rotors (5 * 4 * 3 = 60).
- **Reflector Scope**:
  - **Refl. B**: Standard wartime Reflector B.
  - **Refl. C**: Standard wartime Reflector C.
  - **B or C**: Searches both Reflector B and C spaces consecutively.

### 4. Plugboard Assumptions
- **Unknown**: The plugboard is completely unknown. The Bombe infers stecker leads dynamically through logical constraint loops formed by the crib and ciphertext.
- **Known leads**: Fixes specific known plug pairs and allows the Bombe to deduce the remaining leads.
- **Known board**: Fixes the entire plugboard. Leave the leads field empty to search for an unplugged machine.

### 5. Interactive Execution Controls
- **Run search** checks settings continuously in the background.
- **Pause** or `F6` waits at the next checkpoint. **Resume** continues from there.
- **Step** or `F7` advances to the next checkpoint and waits again. Checkpoints are up to 256 setting checks or plugboard hypotheses apart.
- **Cancel** ends the search and preserves the verified matches it found.

## Scrambler Display {#bombe_scramblers}

Each scrambler shows its rotor model and the starting window at the latest
checkpoint. The line above the drums identifies the rotor and reflector
combination and the crib position being checked. After the search ends,
selecting a match shows its key on the drums.

## Search Telemetry and Results Analysis {#bombe_telemetry}

The panel reports viable crib positions, settings checked, and verified matches. The progress bar tracks setting checks across the selected rotor and reflector combinations. The scrambler display identifies the current combination and crib position. Work and result limits appear in the status line.

### Inspecting Candidate Matches
Browse matches as the solver verifies them:
- Use **Previous** and **Next** to cycle through discovered candidates.
- The details panel reports:
  - Recovered **rotor order**, **reflector**, and **starting windows**, along with the supplied rings.
  - **Crib position** (1-based offset).
  - **Delineated leads**: Plugboard connections confirmed by the constraint loops.
  - **Assumed unplugged sockets**: Sockets that did not participate in crib loops and are tentatively assumed unplugged.
- The ciphertext and plaintext tapes highlight the selected crib in amber.
- Click **Load key** to export the recovered key directly into the active Workbench session.

## Command-Line Bombe CLI (`EnigmaBombe`) {#bombe_cli}

The Bombe search engine is also available as a standalone command-line executable:

### Decipher Syntax
```sh
# Decipher with known settings
mise run bombe -- decipher --ciphertext BDZGO --start AAA --rings 01,01,01

# Decipher with plugboard leads
mise run bombe -- decipher --ciphertext KCHJE --start MCK --rings 01,01,01 --plugs AZ,BY
```

### Search Syntax
```sh
# Search all alignments for a crib with unknown plugboard
mise run bombe -- search --ciphertext BDZGO --crib AAAAA --rings 01,01,01

# Pin specific known leads and test offset 1
mise run bombe -- search --ciphertext KCHJE --crib HELLO --rings 01,01,01 --offset 1 --known-leads AZ,BY

# Search with known unplugged board
mise run bombe -- search --ciphertext BDZGO --crib AAAAA --rings 01,01,01 --plugs ''

# Search all orders of I, II, and III against reflectors B and C
mise run bombe -- search --ciphertext BDZGO --crib AAAAA --rings 01,01,01 --plugs '' --rotors I-III --reflector both

# Read ciphertext from stdin
cat ciphertext.txt | mise run bombe -- search --ciphertext - --crib TANK --rings 02,14,21
```
