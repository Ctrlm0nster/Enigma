/**
 * @page reflector_guide Reflector (Umkehrwalze)
 * @brief Detailed documentation for the reflector component.
 *
 * The Reflector terminates the right-to-left path and bounces current back through the rotor stack.
 *
 * ```mermaid
 * flowchart LR
 *     In[Left Rotor Pin 0] --> R[Reflector B]
 *     R --> Out[Left Rotor Pin 24]
 * ```
 *
 * ## Historical Reflector B Layout
 * ```
 * Input:   A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
 * Output:  Y R U H Q S L D P X N G O K M I E B F Z C W V J A T
 * ```
 *
 * ## Cryptographic Invariant
 * The reflector guarantees:
 * 1. \f$R(x) \neq x\f$ (no letter ever encrypts to itself).
 * 2. \f$R(R(x)) = x\f$ (reciprocity enabling symmetric decryption).
 *
 * ## Class Reference
 * See Reflector for class methods.
 */
