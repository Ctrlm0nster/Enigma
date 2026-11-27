# Decipher and Bombe search {#bombe_guide}

Open **Decipher** in the Workbench header. The Decipher view has its own
ciphertext and results, so you can try settings while keeping your Workbench
message available.

## Decipher with a known key {#bombe_known}

1. Paste the ciphertext into **Ciphertext**. The input keeps up to 240 ASCII
   letters, ignoring spaces and punctuation.
2. Enter the original starting window letters, ring settings, and plugboard
   pairs. Use **Use Workbench key** to copy the current configuration. Give
   each rotor one ring number from **01** to **26**, in left-to-right order.
3. Select **Decipher**. Use **Copy plaintext** to copy all the output letters.

For a quick check, enter `BDZGO`, starting windows `AAA`, rings `01 01 01`,
and an empty plugboard. The plaintext is `AAAAA`.

## Search with a crib {#bombe_search}

Switch to **Bombe search**. A crib is a fragment you expect to find in the
plaintext. The search uses rotors I, II, and III with Reflector B. Enter the
known rings, then give the crib's one-based position in the ciphertext or
leave it blank to try all alignments. Spaces and punctuation do not count
toward the position.

- **Unknown** lets the search infer leads from the crib.
- **Known leads** fixes the pairs you enter and lets the search infer others.
- **Known board** fixes the entire plugboard. Leave the pairs empty for an
  unplugged board.

Select **Run search**. Every result shows starting windows that reproduced
the crib when the ciphertext was replayed. The preview highlights the crib
in amber. If a result lists assumed unplugged sockets, their actual pairings
remain unresolved. Several results can match the same crib. A longer crib or
known leads can narrow the search. The app reports cancellation, a work
limit, and a result limit separately from a completed search.

**Load key** applies the selected preview key to the Workbench, including
its plugboard assumptions. Workbench replays the current message after you
load the key.

## Command line {#bombe_command_line}

```sh
mise run bombe -- decipher --ciphertext BDZGO --start AAA --rings 01,01,01
mise run bombe -- search --ciphertext BDZGO --crib AAAAA --rings 01,01,01 --plugs ''
```

Use `--known-leads AZ,BY` to fix only those pairs, and `--offset 1` to pin
the crib to the first letter. Omit `--plugs` and `--known-leads` to search
with an unknown plugboard. `--ciphertext -` reads ciphertext from standard
input. Press `Ctrl+C` to cancel a running search.
