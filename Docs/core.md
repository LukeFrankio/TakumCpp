# Core Documentation for Takum Library

This document provides an overview of the core functionality in the Takum library, specifically the `takum<N>` template class defined in [`include/takum/core.h`](include/takum/core.h:1). It covers storage, NaR handling, comparisons, basic operations, conversions, and debugging utilities. Refer to [`Docs/Spec.md`](Docs/Spec.md:1) for the full specification and [`Docs/bitlayout.md`](Docs/bitlayout.md:1) for bit layout details. The design adheres to the coding standards in [`Docs/CodingStandards.md`](Docs/CodingStandards.md:1), emphasizing immutability, purity, and functional programming principles.

## Introduction

The `takum<N>` class implements a tapered logarithmic numeric format with N bits of precision (N ≥ 2, ≤ 256). It provides a monotonic total order, NaR (Not a Real) propagation, and conversions to/from host floating-point types. Key concepts:

- **Logarithmic Value (ℓ)**: Represents the value as ℓ = (-1)^S * (c + m), where S is the sign, c is the characteristic, and m is the mantissa fraction. The actual value is exp(ℓ / 2) (base √e).
- **Immutability**: All operations return new instances; no in-place mutation.
- **Purity**: Functions are deterministic and side-effect free, with NaR handling via total ordering.

Use type aliases from [`include/takum/types.h`](include/takum/types.h:1), e.g., `takum32` for `takum<32>`.

## Storage

The internal storage is a packed integer type selected at compile-time:

- For N ≤ 32: `uint32_t`
- For 33 ≤ N ≤ 64: `uint64_t`
- For N > 64: `std::array<uint64_t, ceil(N/64)>`

Bits are packed little-endian across words. Unused high bits are zero. Access raw bits via `storage_t raw_bits() const noexcept` or `static takum from_raw_bits(storage_t bits) noexcept`. This enables bitwise operations while maintaining portability (assumes little-endian host; big-endian via conditional compilation).

See [`Docs/bitlayout.md`](Docs/bitlayout.md:7) for field mapping: S (sign, bit N-1), D (direction, N-2), R (regime, N-5 to N-3), C (characteristic, variable), M (mantissa, lowest p bits where p = N - 5 - r).

## NaR Handling

NaR represents invalid or non-real values (e.g., NaN, Inf, domain errors). Canonical pattern: only sign bit set (S=1, all else 0), i.e., `1ULL << (N-1)`.

- `static takum nar() noexcept`: Factory for NaR.
- `bool is_nar() const noexcept`: Checks for NaR pattern.
- `std::expected<takum, takum_error> to_expected() const noexcept` (C++26) or `std::optional<takum> to_expected() const noexcept` (fallback): Returns unexpected/nullopt for NaR.

Propagation: NaR is smallest in total order (NaR < any real). Operations with NaR yield NaR. Error type `takum_error` enum: `Kind::{DomainError, Overflow, Underflow, InvalidOperation, Inexact, Internal}` with optional message.

Example:
```
takum32 nar_val = takum32::nar();
if (nar_val.is_nar()) { /* handle */ }
auto maybe_val = nar_val.to_expected(); // std::unexpected for NaR
```

## Comparisons

Operators follow total order: NaR smallest, then negatives to positives (monotonic per Proposition 4).

- `bool operator==(const takum& other) const noexcept`
- `bool operator<(const takum& other) const noexcept` (signed integer compare with sign extension)
- `bool operator<=(const takum& other) const noexcept` (derived)
- `bool operator>(const takum& other) const noexcept` (derived)
- `bool operator>=(const takum& other) const noexcept` (derived)
- `bool operator!=(const takum& other) const noexcept` (derived)

NaR == NaR is true; NaR < real is true; real < NaR is false.

## Basic Operations

- `takum operator~() const noexcept`: Bitwise inversion on storage (supports Lemma 3 patterns).
- `takum operator-() const noexcept`: Unary negation via two's complement (~x + 1, Proposition 6); NaR unchanged.
- `takum reciprocal() const noexcept`: 1/x via `to_double()` (Proposition 7); NaR or zero yields NaR.

These are pure, constexpr where possible, and propagate NaR.

## Conversions

- `explicit takum(double x) noexcept`: Encodes double using reference codec (eq. 23-24). Handles NaN/Inf → NaR, clamps |ℓ| < 255.
- `double to_double() const noexcept`: Decodes to double (eq. 24); NaR → NaN.
- `double get_exact_ell() const noexcept`: Extracts exact ℓ = (-1)^S * (c + m); NaR → NaN (placeholder for N > 128).

Encoding (`encode_from_double`): Computes ℓ = 2 * ln(|x|), decomposes to c/m/r, packs S/D/R/C/M. Decoding unpacks and computes exp(ℓ / 2).

Accuracy: Relative error ≤ λ(p) where p ≈ N (see `std::numeric_limits<takum<N>>` for traits like `digits = N`, `epsilon()`).

Static limits:
- `static long double max_ell() noexcept`: Maximum representable |ℓ|.
- `static uint64_t max_finite_storage() noexcept`: Bits for max finite positive.

## Debug and Utilities

- `std::bitset<N> debug_view() const noexcept`: Unpacks bits into bitset (LSB=0).
- `std::numeric_limits<takum::takum<N>>`: Specialized traits (e.g., `is_signed=true`, `has_quiet_NaN=true`, `min()/max()` approximations).

For testing monotonicity/uniqueness, use debug_view and raw_bits with signed integer ordering (start from NaR pattern).

## Error Handling and Expectations

Use `to_expected()` for safe chaining. In C++26, integrates with `std::expected`; fallback uses `std::optional` or `std::variant`.

## Performance Notes

- Inline small ops; constexpr for pure math.
- For N > 64, multi-word ops use loops (optimize later with SIMD).
- Bitwise ops efficient on uint32/64 storage.

See [`Docs/Spec.md`](Docs/Spec.md:190) for full API and Phase 1 details. Examples in [`examples/basic_usage.cpp`](examples/basic_usage.cpp:1).