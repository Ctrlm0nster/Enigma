/**
 * @page plugboard_guide Plugboard (Steckerbrett)
 * @brief Detailed documentation for the plugboard component.
 *
 * The Plugboard sits at the front of the Enigma chassis. Operators inserted double-ended patch cables to swap pairs of letters.
 *
 * ```mermaid
 * graph TD
 *     subgraph Unwired [Unwired Letter Sockets]
 *         B[B] --> B
 *         C[C] --> C
 *     end
 *     subgraph Wired [Patched Wire Pair]
 *         A[A] <-->|Patch Lead| Z[Z]
 *     end
 * ```
 *
 * ## Mathematical Properties
 * - **Involution**: Swapping twice returns the original input: \f$P(P(x)) = x\f$.
 * - **Capacity**: Supports up to 10 patch leads (20 letters), leaving 6 letters self-steckered.
 *
 * ## Class Reference
 * See Plugboard for method specifications.
 */
