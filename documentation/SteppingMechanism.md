# Rotor stepping {#stepping_mechanism}

@tableofcontents

## Before the signal travels {#step_rule}

1. The rightmost rotor is marked to advance on every letter.
2. Each rotor to the right of the first rotor is checked at its current
   window letter. If it is at its turnover notch, mark it and its immediate
   left neighbor to advance.
3. Advance each marked rotor one position. The electrical signal passes
   through the rotors at these new positions.

The simulator applies this rule to all active rotors. A rotor can be marked
by two conditions during the same keypress and still advances one position.

## Double stepping {#double_step}

Start with rotors I, II, III showing **A D V** from left to right. Rotor III
is at its turnover notch **V**, so it carries Rotor II from **D** to **E**.
Rotor II is now at its own notch and moves again on the next press, carrying
Rotor I with it.

| Press | Window letters | Rotors that move |
| --- | --- | --- |
| Start | **A D V** | |
| First | **A E W** | II, III |
| Second | **B F X** | I, II, III |

This is the double step. Rotor II advances on two successive keypresses.
Try starting positions **A D V** in the Workbench and watch the **Live**
column as you type two letters. @ref rotor_guide "Turnover positions for I–V"
