% Phase 1 — TakumCpp specification

This document completes Phase 1 of the TakumCpp plan: research and specification. It captures the constraints, design decisions, API contract, pseudocode for encoding/decoding, LUT/poly strategy for Gaussian-log (Φ), storage layout, NaR handling via std::expected, testing checklist, and next steps.

## Summary

- Project: TakumCpp — a C++26 library implementing the Takum (tapered logarithmic) numeric formats.
- Phase 1 goal: Produce a clear, implementation-ready specification (`Docs/spec.md`) that all subsequent phases implement against.
- Assumptions: minimum C++ standard is C++26 (with guarded fallbacks for C++23 compilers), optional `<stdfloat>` availability, target platforms: x86_64 and common compilers (MSVC/GCC/Clang).

## Phase 1 checklist (requirements and status)

- [x] Produce `Docs/spec.md` (this file) covering all Phase 1 deliverables.
- [x] Confirm existing test placeholder `test/placeholder.test.cpp` exists and remains a scaffold for Phase 2+.
- [x] Specify encoding/decoding pseudocode (`tau`/`tau_inv`) and storage packing.
- [x] Define NaR propagation strategy and `std::expected` API surface.
- [x] Specify LUT / polynomial strategy for Φ (Gaussian-log) for takum16/32/64+.
- [x] Provide a minimal test plan and verification checklist for Phase 1 outputs.

If anything below is under-specified, the implementation will note assumptions explicitly and proceed.

## High-level contract (short)

- Inputs: usual C++ arithmetic types and takum binary representation.
- Outputs: `takum<N>` values, `std::expected<takum<N>, takum_error>` for NaR-aware APIs, conversions to/from host floats (`float`, `double`, `long double`, and `<stdfloat>` types when available).
- Error modes: NaR (Not a Real) encoding; invalid operations yield `takum_error` via `std::expected` in safe APIs. Unsafe APIs may throw or produce NaR depending on chosen policy.
- Success criteria: deterministic, bit-exact packing/unpacking of encodings per pseudocode; round-trip conversion budget documented (λ(p) tolerance); compile-time checks for storage size.

## Key assumptions (inferred)

1. C++26 is available; `std::expected` is available (or a thin shim will be provided if not).
2. `<stdfloat>` may not be present on all compilers; interop is optional and guarded by feature-test macros.
3. Targeted numeric range and properties follow the Takum paper (tapered logarithmic LNS) — dynamic range and tapering parameters will be constants in the implementation and documented here.
4. NaR behaves as an orthogonal non-value that propagates through operations unless explicitly handled via `std::expected` APIs.

## Types, storage, and packing

Design goals:

- Use fixed-size, packed integer storage for speed and easy bitwise ops.
- Provide an optional debug view using `std::bitset<N>`.
- Provide compile-time selection of storage type based on N.

Storage rules:

- For N <= 32: storage_t = `uint32_t` (lower bits used; unused high bits = 0).
- For 33 <= N <= 64: storage_t = `uint64_t`.
- For N > 64: storage_t = `std::array<uint64_t, K>` where K = ceil(N/64).

Layout (logical fields — conceptual; exact bit assignment is implementation detail but must be stable and documented):

- sign bit (1 bit): reserved for sign/NaR marker depending on encoding variant.
- regime/exponent/log-mantissa fields: tapered layout per paper.
- NaR canonical pattern: a reserved bit-pattern (documented constant) that unambiguously denotes 'Not a Real'.

Debug API:

- `std::bitset<N> debug_view() const;` — read-only view for tests.

Packing invariants:

- Bit-packing is little-endian on storage words; all public bit-twiddling helpers must be endian-aware and use portable bit-extraction helpers.

Rationale: uint32/64 backing allows efficient bitwise ops, easy use of SIMD lanes later, and compact LUT indexing.

## NaR and error handling

Design:

- `takum<N>` is a value type that may represent NaR.
- Public, fallible operations expose safe variants using `std::expected<takum<N>, takum_error>`.
- Convenience helpers:
  - `bool is_nar() const noexcept;`
  - `std::expected<takum<N>, takum_error> to_expected() const noexcept;`
  - `takum<N> nar()` static factory returning canonical NaR.

`takum_error`:

- A lightweight error type containing an enum of common error reasons (DomainError, Overflow, Underflow, InvalidOperation, Inexact, Internal) plus an optional message.

Propagation rules:

- If any operand is NaR, most arithmetic functions return NaR (and in safe APIs return `std::unexpected(takum_error{InvalidOperation})`).
- Implementations may provide both unchecked APIs (fast, produce NaR or saturate per policy) and checked APIs returning `std::expected`.

NaR Total-Ordering Policy:
- The NaR follows a total-ordering convention where NaR = NaR and NaR is the smallest element in comparisons (NaR < x for all real x).
- Comparison operators ( <, <=, >, >=, ==, != ) treat NaR as smallest: NaR < x = true for real x, x < NaR = false; NaR == NaR true.
- std::expected wrappers: to_expected() returns unexpected for NaR.
- Sorting: std::sort on takum vectors places all NaR at the beginning (smallest), reals in monotonic order.
- This ensures total order for bitwise comparisons as two's complement signed integers, with NaR bit pattern (1 << (N-1)) as smallest.

Design note: `std::expected` lets callers choose to propagate NaR as an error or recover with fallback policies.

## Encoding/decoding pseudocode (tau / tau_inv)

Purpose: provide clear, implementation-ready pseudocode for converting between external floating types (double, float, std::stdfloat types) and the packed takum bit representation.

Terminology used below:

- x: input real value (floating point)
- N: total number of bits in takum representation
- p: precision parameter (number of significand/log-mantissa bits — implementation-chosen per N)
- tau(x): encode real x → takum bits
- tau_inv(bits): decode takum bits → real x (or a log-domain representation)

Pseudocode: encoding (tau)

**Warning: ℓ vs τ Confusion**: The full τ = (S, ℓ) where ℓ is the logarithmic value (c + m / 2^p). Injectivity/uniqueness is for τ, not ℓ alone (per Definition 2 and proof). Different signs or (c,m) can yield same ℓ numeric but distinct τ. All tests must compare full τ or canonical decoded form (e.g., to_double()), never ℓ in isolation.

1. If x is NaN or signaling-NaN: return takum::nar().
2. If x is zero: return canonical zero encoding (sign=0, rest=0).
3. sign = (x < 0) ? 1 : 0
4. y = abs(x)
5. If y is infinite: return takum::nar() or saturate to max representable (policy choice). Document the chosen policy. (Recommended default: NaR.)
6. Compute logarithmic value ℓ = sign_adjusted_log(y) — for Takum, base is sqrt(e) or another predetermined base per design; use a high-precision host type (long double) for intermediate.
7. Determine regime and tapered mantissa from ℓ according to the tapered-log encoding rules (regime length, exponent bits, mantissa bits). This step maps ℓ → (regime, exponent, fraction) where regime encodes coarse scaling and fraction encodes fine-grain value within regime.
8. Pack sign, regime, exponent, fraction into storage_t following the stable bit layout.
9. Return packed bits as `takum<N>`.

Pseudocode: decoding (tau_inv)

1. If bits == takum::nar_pattern: return NaR.
2. Unpack sign, regime, exponent, fraction from storage bits.
3. Reconstruct ℓ (log-domain value) from fields: ℓ = regime_to_scale(regime) + exponent * exp_unit + fraction * frac_unit.
4. Compute y = exp_base(ℓ) where exp_base is the inverse of sign_adjusted_log; again compute in a high-precision host type to minimize rounding error.
5. If sign bit set, return -y, else +y.

Canonical Decoded Tuple (S, c, r, m_int):

For verification and testing (e.g., uniqueness in Proposition 3, monotonicity in Proposition 4), the decoded bit pattern can be unpacked into the tuple (S, c, r, m_int):

- S: sign bit (0 or 1)
- c: characteristic (integer part of ℓ, per eq. (19))
- r: regime value (per eq. (17))
- m_int: packed mantissa bits (lowest p = N - 5 - r bits, as uint64_t)

This tuple provides an exact, canonical representation of the Takum value for bit-level verification without floating-point rounding issues. The tuple can be used to compute the logarithmic value ℓ = (-1)^S * (c + m_int / 2^p) for high-precision comparisons in tests.

**Note on Ordering for Tests**: Two's-complement ordering maps to SI order for ascending τ: iteration should start at nar_index = 1<<(n-1), then nar_index+1 .. 2^n-1, 0 .. nar_index-1. Unsigned 0..2^n-1 does not correspond to ascending τ; reordering is required for monotonicity checks (Prop 4) to avoid false failures.

Implementation notes:

- Use constexpr helpers for bit shifts/masks where possible.
- Keep the tau and tau_inv implementations small and auditable (single-responsibility helpers: extract_fields, pack_fields, regime_to_scale, scale_to_regime).
- For small N and test harnesses, provide a slow but obvious reference implementation (pure integer + long double math) for unit tests and cross-checking.

Reference invariants to test:

- round_trip: for selected host floats v, tau_inv(tau(v)) should be within documented λ(p) bound.
- uniqueness: distinct representable reals map to distinct encodings and vice-versa where the format claims uniqueness.

## Gaussian-log (Φ) strategy: LUT + interpolation / hybrid (LUT + minimax poly)

Goal: implement an efficient approximation of Gaussian-log (Φ_b^±) used for addition/subtraction in the log domain.

Strategy summary:

- takum16/takum32: use dense LUT + interpolation. Rationale: small precisions benefit from table-driven approximations.
  - takum16: LUT entries: 1024; interpolation: linear fixed-point by default; cubic fixed-point optional for higher accuracy.
  - takum32: LUT entries: 4096; interpolation: linear by default; cubic optional.
- takum64 and up: hybrid approach.
  - coarse LUT: 256 entries for coarse indexing.
  - polynomial: degree 5..7 minimax polynomials on each coarse interval. Coefficients pre-generated by an offline script (see `scripts/gen_poly_coeffs.py`).

Representation and interpolation details:

- LUT entries stored as fixed-point integers (e.g., int32_t or int64_t) with a compile-time chosen fractional bits (Q format). This keeps tables small and usable in constexpr contexts.
- Interpolation:
  - Linear: fast, low code size. Good baseline.
  - Cubic (Hermite or cubic B-spline): provides better accuracy for takum16 in exchange for a few extra multiplies.
- For hybrid polynomials: store polynomial coefficients in constexpr arrays per interval. Use Horner's method for evaluation.

Accuracy targets and sizes (recommended):

- takum16: 1024-entry LUT, linear interp → target mean error << 1 ulp of takum16; cubic interp optional.
- takum32: 4096-entry LUT, linear interp → target mean error << 0.1 ulp of takum32.
- takum64+: 256-entry LUT + minimax polynomials degree 5..7 → target max error within λ(p) tolerance for respective N.

Space/time tradeoffs:

- LUT-only: more memory, faster, simpler.
- Hybrid: smaller memory use, slightly more compute, better asymptotic scaling to higher N.

Offline tooling:

- Provide `scripts/gen_poly_coeffs.py` to produce polynomial coefficients for the hybrid approach. Coeffs are generated offline and checked into `include/takum/internal/generated/` as `constexpr` arrays.

## API surface (Phase 1 scope)

Files and primary declarations (Phase 1 defines interfaces and traits; implementations in Phase 2+):

- `include/takum/core.h` — `template<size_t N> struct takum;` constructors, `is_nar()`, `to_expected()`, `debug_view()`, `static takum nar()`.
- `include/takum/traits.h` — `takum_traits<N>` containing λ(p), dynamic range, storage_t, bits layout constants.
- `include/takum/compatibility.h` — declared shims and planned #pragma messages for deprecated C APIs.
- `include/takum/internal/phi_spec.h` — LUT sizes, Q-formats, polynomial degrees, and generated coeffs placement.

Safe/unsafe API pattern:

- fast unchecked: `takum<N> add_unchecked(takum<N> a, takum<N> b) noexcept;` (may produce NaR or saturate per policy)
- checked: `std::expected<takum<N>, takum_error> add(takum<N> a, takum<N> b) noexcept;`

Naming conventions:

- types and files under `takum/*` namespace and include path.
- functions mirror `<cmath>` names (`takum::sin`, free-function `sin(takum<N>)`) and provide `sinf`/`sinl` shims via `compatibility.h`.

## Test plan and verification checklist (Phase 1 outputs)

Phase 1 is a spec step; produce tests primarily to verify the reference encoder/decoder (small reference implementation) and packing invariants.

Unit tests to write in Phase 2/Phase 7 (listed here so Phase 2 can rely on them):

1. Encoding/decoding round-trip test: sample set of host floats (edge cases: 0, subnormals, denormals if supported, near-largest, ±Inf, ±NaN) and assert tau_inv(tau(x)) within λ(p) bound using high-precision long double decoded values/ℓ to avoid rounding collapses. **Test Guideline: Full τ Comparison**: Verify round-trip using full τ (including sign), not ℓ; use high-precision long double canonical decoded form for comparison.
2. Canonical NaR test: ensure NaN host float encodes to `takum::nar()` and decodes back to a canonical NaR branch.
3. Packing invariants: bit-width, storage_t selection, and debug_view correctness.
4. Uniqueness properties: check that two distinct canonical real values (within representable set) produce different encodings where the format guarantees uniqueness.
5. Regression checks: small table of known example values from the Takum paper (if any simple numerical examples are present) and expected encodings/decodings.

Smoke tests for Phase 1:

- Add `test/spec_reference.test.cpp` later that includes a tiny, reference (non-optimized) tau/tau_inv implementation and validates round-trip for a focused set.

## Deliverables for Phase 1

- `Docs/spec.md` (this file) — complete spec and decisions.
- A small reference implementation (kept in `include/takum/internal/ref/`) is recommended for Phase 2; Phase 1 will not add it to the repo but documents its need.
- Test skeleton exists at `test/placeholder.test.cpp` (confirmed); add `test/spec_reference.test.cpp` in Phase 2.

## Minimal timeline and next steps (for Phase 2 kickoff)

1. Implement `include/takum/core.h` template skeleton and `takum_traits<N>` (Phase 2 step 1).
2. Create a small reference encoder/decoder in `include/takum/internal/ref/` for unit testing.
3. Add `test/spec_reference.test.cpp` verifying round-trip and NaR invariants.
4. Iterate on precise bit layout and finalize constants (regime width, p mapping for each N).

## Open decisions (to be finalized before coding)

- NaR vs saturate policy for infinities: recommended default: NaR, but make policy configurable via traits.
- **Ordering for Monotonicity Tests**: Explicitly document SI reordering in test harnesses (start at 1<<(n-1), wrap); add helper function to generate SI-ordered indices for Prop 4 tests.
- Exact bit layout and canonical NaR pattern: choose a pattern that is uncommon and simple (e.g., top bits all ones + sign), then document it in `takum_traits`.
- Base of the logarithm: paper suggests √e; document the choice and keep it as a compile-time constant.

## Appendix: short TODO for Phase 2

- Add `include/takum/core.h` skeleton and feature-test guards for `<stdfloat>`.
- Implement storage selector metafunction (N -> storage_t).
- Implement `is_nar()` and `nar()` pattern constant.
- Implement a small reference `tau` / `tau_inv` in `include/takum/internal/ref/tau_ref.h` for unit testing.

## Appendix: C++26 features used and fallback strategy

This project prefers C++26 features where they simplify implementation and enable stronger compile-time guarantees. The following features are used when available; all uses are guarded with feature-test macros and have fallbacks to keep the project usable with C++23 toolchains.

- Reflection / std::meta: Used to generate per-precision metadata and to assist in compile-time LUT/polynomial generation. Fallback: offline code generation (`scripts/gen_poly_coeffs.py`) producing `include/takum/internal/generated/*`.
- Extended NTTPs and library approaches to constant template parameters: used to encode LUTs/coeffs as NTTPs for faster compile-time access. Fallback: pregenerated constexpr arrays included from `include/takum/internal/generated/`.
- Stronger constexpr/consteval math facilities: prefer compile-time poly generation and evaluation. Fallback: runtime evaluation or pregenerated coefficients.
- `std::format` / `std::runtime_format` improvements: used for improved IO and `to_string` behavior in `include/takum/io.h`. Fallback: use existing `std::to_chars`/`std::ostringstream` with documented differences.

Build flags and CMake options (recommended):

- `-DTAKUM_REQUIRE_CXX26=ON` — fail configuration if compiler does not support required C++26 features.
- `-DTAKUM_USE_COMPILETIME_COEFFS=ON` — enable compile-time LUT/poly generation (requires reflection/consteval support); otherwise the build uses pregenerated tables.

The repository keeps an offline script (`scripts/gen_poly_coeffs.py`) and checked-in generated headers so the library remains usable on older toolchains.

---

End of Phase 1 specification. This file is intentionally actionable and implementation-focused to make Phase 2 straightforward.
