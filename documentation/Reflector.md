# Reflector B {#reflector_guide}

The reflector sends the signal back through the rotor bank. Reflector B
has 13 fixed pairs. A signal entering on `A` leaves on `Y`, and a signal
entering on `Y` leaves on `A`.

## Wiring {#reflector_wiring}

```text
Input:   A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
Output:  Y R U H Q S L D P X N G O K M I E B F Z C W V J A T
```

The reciprocal pairs let the machine decipher text with the same setup it
used to encipher it. Each reflector pair connects two different contacts,
so no key can light its own letter. @ref Reflector "Reflector API"
