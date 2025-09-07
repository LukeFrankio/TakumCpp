# Canonical Bit Layout for Takum<N>

This document specifies the exact bit layout for the `takum<N>` storage, ensuring stable, unambiguous packing. Bits are numbered from 0 (LSB) to N-1 (MSB). The layout follows the tapered logarithmic encoding as defined in the specification.

## Field Mapping

| Bit Index | Field | Width | Description |
|-----------|-------|-------|-------------|
| N-1 | S | 1 | Sign bit: 0 for positive/zero, 1 for negative or NaR (when all other bits are 0). |
| N-2 | D | 1 | Regime direction: 1 for positive regime (run of 1s), 0 for negative regime (run of 0s). |
| N-5 to N-3 | R | 3 | Regime length code: r = D ? R : (7 - R), where r is the regime run length (0 to 7). |
| N-5-r to N-6 | C | r | Characteristic bits: The r bits following R encode the characteristic value c. For D=1: c = (1 << r) - 1 + C; For D=0: c = -((1 << (r+1)) - 1 - C). |
| 0 to p-1 | M | p | Mantissa bits: p = N - 5 - r fractional bits, m = M / 2^p, where ℓ = (-1)^S * (c + m). |

- **NaR Pattern**: S=1 and all other bits=0 (i.e., storage with only bit N-1 set). This is the canonical "Not a Real" representation.
- **Zero**: All bits=0.
- **Uniqueness**: Distinct bit patterns (except NaR) map to unique τ = (S, c, m) tuples, ensuring total ordering and monotonicity in signed integer interpretation.

## Endianness and Packing for N > 32

- **Storage Type**: For N > 64, `storage_t` is `std::array<uint64_t, ceil(N/64)>`.
- **Word Packing**: Bits are packed little-endian across words: the lowest 64 bits in storage[0], next 64 in storage[1], etc. Unused bits in the highest word are 0.
- **Within Word**: Bits are little-endian (bit 0 is LSB of storage[0]).
- **Endianness**: Assumes host little-endian for portability; for big-endian hosts, use byte-swapped access or define TAKUM_BIG_ENDIAN. However, the project targets x86_64 (little-endian). Use [`std::bit_cast`](<cstdint>:0) for round-trip verification, which preserves native representation.
- **Multi-Word Example for N=128**: Bits 0-63 in storage[0], 64-127 in storage[1]. To access bit k: word = k / 64, bit_in_word = k % 64; (storage[word] & (1ULL << bit_in_word)) != 0.

## Verification

- **Round-Trip Invariant**: For any `takum<N> t`, `t.debug_view().to_string()` should match the bit representation extracted via bit shifts, and reconstructing via `storage = bit_cast` should preserve all bits.
- **Tests**: See [test/core.test.cpp](test/core.test.cpp:1) for assertions using `debug_view()` and manual bit extraction.
- **Debug View**: `t.debug_view()` returns `std::bitset<N>` with bits set from LSB (index 0) to MSB (index N-1).

This layout ensures stable bit-packing across compilers and platforms, with "ghost bits" (unused high bits for N<32/64) always 0.