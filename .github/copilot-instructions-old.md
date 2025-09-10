# TakumCpp AI Assistant Instructions

Purpose: Make AI coding agents immediately productive when contributing to TakumCpp (logarithmic tapered floating‑point library). Keep changes minimal, pure, and standards‐conformant.

## Core Domain Model
- Central type: template< size_t N > `takum<N>` defined in `include/takum/core.h` (header‑only, immutable). Storage is packed: N<=32 -> uint32_t, N<=64 -> uint64_t, else array<uint64_t>.
- Special value: NaR (Not‑a‑Real) = sign bit set, all other bits zero; always propagates. Treat NaR as smallest in total ordering (`operator<`).
- Encoding fields (S D R C M) per spec; all current arithmetic still uses host double intermediates except emerging Φ (Gaussian‑log) path for improved +/−.
- Conversions: constructor from double; `to_double()`, `get_exact_ell()` (diagnostic ℓ = 2 ln |x|); `from_ell(sign, ell)`; `nar()`, `is_nar()`, `minpos()`.

## Arithmetic Layer (`include/takum/arithmetic.h`)
- Phase‑3 implementation: + − * / use double intermediates; addition has an optional fast Φ heuristic guarded by `TAKUM_ENABLE_FAST_ADD` macro.
- Safe variants: `safe_add/sub/mul/div/abs/recip` return `std::expected` (C++26/23) or `std::optional` fallback; never throw.
- Negation flips sign bit; division by zero -> NaR; any NaR operand -> NaR.

## Gaussian‑log (Φ) Infrastructure
- Internal headers under `include/takum/internal/`: `phi_eval.h`, `phi_lut.h`, `phi_types.h`, generated coeffs in `internal/generated/phi_coeffs.h` (created via `scripts/gen_poly_coeffs.py`).
- Dispatch: for N<=16 uses 1024‑entry LUT (`phi_lut_1024`), N<=32 uses 4096 LUT, else hybrid polynomial (`phi_hybrid_eval`). Current LUTs may be placeholders—preserve interfaces.
- When touching addition logic, call `takum::internal::phi::phi_eval<N>(t)`; do not hard‑code polynomial details.

## Build & Tooling
- Header‑only library target: `TakumCpp` (INTERFACE). Root `CMakeLists.txt` negotiates highest supported standard: prefers C++26, falls back to 23/20. Do not introduce code requiring newer than selected `CMAKE_CXX_STANDARD`.
- Generated file dependency: tests and examples depend on `phi_coeffs_gen`; keep custom command intact if adding new generated headers.
- Tests live in `test/` and are auto‑collected (`file(GLOB *.test.cpp)`). Add new tests as `*.test.cpp`. Link only headers (no sources) unless adding new non‑template utilities.
- To build (Windows / PowerShell typical):
  1. `cmake -S . -B build` (or use existing tasks).
  2. `cmake --build build`.
  3. `ctest --test-dir build --output-on-failure`.
 - Recommended one‑step workflow on Windows: run `./build.bat` (wraps `build.ps1`) which: (a) regenerates Φ coeffs if Python present, (b) configures with Ninja or MinGW make, (c) builds in `build/`, (d) runs tests with failure output, (e) optionally runs Doxygen.

## Conventions & Constraints
- Immutability: `takum<N>` values are conceptually immutable; no in‑place mutating ops (avoid operator+= etc.).
- Error signaling: prefer returning NaR or `std::expected` safe variants; never throw except unrecoverable (currently none implemented).
- NaR semantics: treat as absorbing for arithmetic; ordering: NaR < any real, NaR == NaR.
- Avoid exceptions, dynamic allocation, RTTI. Keep everything `constexpr` where practical (respect current use patterns in `core.h`).
- Keep multi‑word (N>64) correctness: use helper `mask_to_N`, sign bit calculations—do NOT assume single 64‑bit path.
- Follow `Docs/CodingStandards.md` (2‑space indent, ≤100 char lines, Doxygen `///` for public APIs). Preserve existing naming (snake_case functions, PascalCase types not yet widely used beyond `takum_error`).

## Adding / Modifying Features
- Prefer extending `core.h` or new internal headers; avoid splitting template definitions into .cpp.
- For new math operations: start with double intermediate for correctness, add TODO noting future native ℓ/Φ implementation.
- Any new encoding/decoding helpers must maintain NaR pattern and monotonic total order (see comparison code in `core.h`). Reuse existing sign‑extension pattern for N<=64 and lexicographic path for multi‑word.
- When generating or consuming Φ coefficients, update the custom command only if absolutely necessary; keep signature: script input = output header path.

## Testing Expectations
- Add focused tests: arithmetic invariants (NaR propagation, monotonic ordering), edge encodings (minpos, nar(), max boundary). Use existing patterns in `arithmetic.test.cpp`.
- For multi‑word (e.g., 128‑bit) add tests using `takum128` aliases (`include/takum/types.h`).

## Compatibility & Shims
- `compatibility.h` contains deprecated aliases (`float8_t`) and expected shim; do not expand without strong reason. New shims must emit `#pragma message`.
- Maintain dual path for C++26 expected vs. optional fallback (guard on `__cplusplus >= 202302L`).

## Performance Hooks
- `TAKUM_ENABLE_FAST_ADD` macro gate for experimental faster addition path; keep code guarded and side‑effect free.
- `TAKUM_ENABLE_CUBIC_PHI_LUT` enables Catmull‑Rom cubic interpolation for small Φ LUTs.
- `TAKUM_COARSE_LUT_SIZE` controls hybrid coarse LUT size (default 256) for takum64+ Φ path.
- `TAKUM_ENABLE_PHI_DIAGNOSTICS` gathers lightweight per‑precision counters (non‑atomic); do not introduce heavy instrumentation.
- Future SIMD/residual optimizations will go into `optimized.h` (not yet present); keep new perf work isolated and trivially removable.

## Typical Change Workflow (AI)
1. Locate relevant header(s); gather context (core + internal if arithmetic/math change).
2. Implement change with minimal diff; preserve public signatures.
3. Add/adjust tests (`*.test.cpp`).
4. Ensure generated coeff header logic unaffected (don’t commit large binary artifacts).
5. Build & run tests; fix warnings (treat as errors in stricter builds later).

## Pitfalls to Avoid
- Don’t introduce stateful singletons or globals—keep pure.
- Don’t assume IEEE 754 behaviors (NaR != NaN; use `is_nar()` not `std::isnan`).
- Don’t bypass existing encode/decode when constructing from host values; use provided helpers.
- Don’t write platform‑specific intrinsics directly in core paths (add gated section later if needed).

## Quick Reference Key Files
- `include/takum/core.h`: type definition, encoding/decoding, ordering.
- `include/takum/arithmetic.h`: basic ops, safe variants.
- `include/takum/internal/phi_eval.h`: Φ evaluation dispatch.
- `include/takum/compatibility.h`: shims & legacy.
- `include/takum/types.h`: common aliases.
- `scripts/gen_poly_coeffs.py`: coefficient generation (custom command).
- `test/*.test.cpp`: GoogleTest suites (pattern for adding new tests).

If anything here seems outdated (e.g., native Φ addition fully implemented), flag and update this file accordingly.
