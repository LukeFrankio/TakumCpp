# Coding Standards for Takum Arithmetic Library

This document outlines the coding standards for the Takum library project. The goal is to ensure code is readable, maintainable, performant, and aligned with functional programming (FP) principles (immutability, purity, composability). Standards are based on the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with adaptations for FP (e.g., no in-place mutation) and Takum specifics (logarithmic arithmetic, NaR handling). Violations should be justified in comments. Use clang-format for auto-formatting (`.clang-format` file in repo root).

## 1. Language and Compiler
- **C++ Standard**: C++26 (or later; use `-std=c++26`). Fallback to C++20 for MSVC (via SFINAE for concepts).
- **Compiler**: GCC/Clang 14+ (full C++26 where available); MSVC 2022+ (preview). Warn on non-compliant features.
- **Build**: CMake 3.20+. No external deps except standard lib; optional MPFR for validation.
- **Warnings**: Treat as errors (`-Wall -Wextra -Werror -pedantic`). Enable sanitizers (ASan/UBSan) in debug.

## 2. Naming Conventions
- **Types**: PascalCase (e.g., `Takum32`, `ComplexTakum`).
- **Functions/Variables**: snake_case (e.g., `add_takum`, `takum_value`).
- **Constants**: UPPER_SNAKE_CASE (e.g., `PI_TAKUM`).
- **Templates**: PascalCase (e.g., `template<size_t N> struct Takum;`).
- **FP-Specific**: Suffix functors "_t" (e.g., `plus_t`); monads "_maybe" (e.g., `takum_maybe`).
- **Files**: snake_case.h (e.g., `arithmetic.h`); one public class/header.

## 3. Code Structure and Organization
- **Headers**: `.h` only (no .cpp for templates; inline where possible). Include guards `#pragma once`. Self-contained (forward-declare deps).
- **Includes**: `<header>` for standard; `"takum/..."` for project. Sorted: standard, project, self.
- **Namespaces**: `namespace takum { ... }`; sub-ns for groups (e.g., `namespace takum::math { ... }`).
- **Indentation**: 2 spaces (no tabs). Braces on new line (K&R style).
- **Line Length**: 100 chars max.
- **Comments**: Doxygen for public API (///); inline // for logic. Explain Takum specifics (e.g., "NaR propagates per Definition 7").
- **Preprocessor**: Minimal; no macros except shims (e.g., `#define sinf takum_sinf`). Use constexpr instead.

## 4. Functional Programming Principles
- **Immutability**: All Takum types immutable (const members; no setters). Ops return new instances (e.g., `takum add(takum a, takum b);` not `a += b;`).
- **Purity**: All functions pure: No side effects, globals, I/O (except explicit io.h). Deterministic (same input → same output; handle NaR consistently).
- **Composition**: Prefer lambdas/ranges (e.g., `x | std::views::transform(sin)`). Higher-order funcs (e.g., `map_f<F>(F f)`). No recursion unless tail (avoid stack overflow).
- **Error Handling**: Use `std::expected<takum<N>, na_r_error>` for NaR (C++26); fallback `std::optional` (C++17). No exceptions (throw only in unrecoverable, e.g., invalid N<12).
- **Monads/Applicatives**: For NaR/state: `takum_maybe` with bind/map. Test monad laws.
- **Deprecated Shims**: Prefix "shim_" (e.g., `shim_sinf`); #pragma message("Deprecated: Use takum_sin").

## 5. Takum-Specific Rules
- **Precision**: Template<size_t N> (N>=12); use aliases (e.g., `using takum32 = takum<32>;`). Constexpr N where possible.
- **Bit Representation**: Use `std::bitset<N>` internally; expose via bit_cast. Monotonic: Ensure Prop 4 (tests enforce).
- **Log Domain**: Ops in ℓ-space (e.g., mul = add_ell); comment Gaussian logs (Φ^± with refs).
- **NaR/0/Saturation**: Propagate NaR (total-ordering); saturate overflows (Algorithm 1). Specialize limits (infinity=NaR).
- **Accuracy**: Rel err <= λ(p) (Prop 11); assert in debug (e.g., `static_assert(rel_err < lambda_p(N));`).
- **Deprecated Coverage**: Every deprecated feature has a shim (e.g., subnormals → saturate; old NaN pow → NaR). Test 100%.

## 6. Performance and Safety
- **Inlining**: Inline small pure funcs (constexpr); use [[nodiscard]] for returns.
- **Constexpr**: All pure math/arithmetic constexpr (C++26 relaxations for logs).
- **SIMD/Opt**: Use intrinsics in optimized.h; profile with -O3. Avoid branches for NaR (bit checks).
- **Security**: Bound loops (no infinite); fuzz inputs. No UB (e.g., log(0) → NaR, not -inf).
- **Portability**: No asm/arch-specific except optional (e.g., #ifdef __AVX2__); test x86/ARM/Windows/Linux.

## 7. Testing
- **Unit**: Google Test; 100% coverage (gcov/lcov). Property-based (e.g., quickcheck for monotonicity).
- **Integration**: Full feature matrix, including deprecations.
- **Perf**: Google Benchmark; target < IEEE latency for mul/div.
- **Style**: clang-tidy; enforce in CI.

## 8. Documentation
- **Doxygen**: All public; @tparam for templates, @note for FP/Takum specifics.
- **Examples**: Self-contained .cpp with #include.
- **Deprecations**: Document shims with migration paths.

## 9. Commit and Review
- **Commits**: Atomic (one feature/fix); descriptive messages (e.g., "Add sin overload for takum<N>").
- **Reviews**: PRs required; check FP purity, Takum props.
- **Versioning**: Semantic (v1.0.0); changelog for deprecations.

Enforce via .clang-format, .clang-tidy, pre-commit hooks. Questions? Open issue.