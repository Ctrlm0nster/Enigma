# Plugboard {#plugboard_guide}

The plugboard swaps letters before the signal enters the rotors and again
when it returns to the lamp. A lead between **A** and **Z** sends **A** to
**Z** and **Z** to **A**. An unused socket passes its letter through.

## Workbench and core {#plugboard_implementation}

The Workbench starts with **A–Z** and **B–Y** and supports up to ten leads.
See the socket controls in the @ref workbench_guide "Workbench guide".

The @ref Plugboard class starts with no pairs. It stores a 26-entry array
that maps each contact directly to its paired contact or to itself. Each
call to `addWire` requires two different, unused contacts and fills in both
directions of the pair.
