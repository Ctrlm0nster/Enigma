/**
 * @page charactermap_guide Charactermap Module
 * @brief Detailed guide for uppercase ASCII character arithmetic.
 *
 * The Charactermap module provides fast, boundary-checked conversion routines between ASCII letter characters
 * and integer pin numbers.
 *
 * ```mermaid
 * graph LR
 *     Char['A'..'Z'] -->|charToIndex| Index[0..25]
 *     Index -->|indexToChar| Char
 *     Index -->|idx| SizeT[std::size_t Index]
 * ```
 *
 * ## API Reference
 * - charToIndex(char c)
 * - indexToChar(int i)
 * - idx(int i)
 */
