# Character mapping {#charactermap_guide}

The core represents contacts as integers from `0` to `25`. `A` is `0`
and `Z` is `25`. The Workbench uses the same conversion to label keys,
sockets, and signal stages.

| Function | Input and result |
| --- | --- |
| @ref charToIndex | Converts an uppercase ASCII letter to a contact index. |
| @ref indexToChar | Converts a contact index to an uppercase ASCII letter. |
| @ref idx | Casts a contact index to `std::size_t` for array indexing. |

The functions expect inputs in those ranges and use assertions to check them
when assertions are enabled.
