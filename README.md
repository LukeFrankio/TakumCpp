# TakumCpp: A Logarithmic Tapered Floating-Point Arithmetic Library

TakumCpp is a C++ library implementing the Takum arithmetic format as described in the research paper. It provides a high-performance, logarithmic tapered floating-point system with properties like monotonicity, total-ordering, and NaR (Not a Real) handling. The library prefers C++26 and leverages `std::expected` for error propagation, packed integer storage for efficiency, and full coverage of C++ floating-point features including deprecations; guarded fallbacks keep the project usable on C++23 toolchains.

## Features
- Template-based precision: `takum<N>` for N ≥ 12 bits.
- Immutable, pure functions following functional programming principles.
- Gaussian-log approximations for addition/subtraction using LUT + interpolation or hybrid polynomials.
- Compatibility shims for deprecated C++ features with compile-time warnings.
- Integration with standard library components like `<cmath>`, `<complex>`, `<random>`, etc.
- `<stdfloat>` interop where available.

## Development Progress
The library is currently halfway through Phase 4 of the implementation plan (Mathematical Functions). Core type, encoding/decoding, traits, and basic arithmetic operators (+, -, *, /, abs, comparisons) are complete. Gaussian-log (Φ) approximations for addition/subtraction are implemented, but full `<cmath>` equivalents (trig, exp/log, pow, rounding, classification) remain pending.

## Implemented Floating-Point Replacements
Based on the current codebase, the following C++ floating-point features have been replaced or partially implemented:
- Built-in types (`float`, `double`, `long double`) → `takum32`, `takum64`, `takum128` aliases and `takum<N>` template (`[types.h](include/takum/types.h)`, `[core.h](include/takum/core.h)`).
- Literals and constructors from `double`/`float` (lossy conversion via encoding).
- Basic operators: `+`, `-`, `*`, `/`, unary `-`, `abs`, comparisons (`<`, `==`, etc.) via pure functions (`[arithmetic.h](include/takum/arithmetic.h)`, `[core.h](include/takum/core.h)`).
- `<limits>`: `std::numeric_limits<takum<N>>` specialization (min/max/epsilon/infinity/NaN traits).
- `<bit>`: `bit_cast` equivalents via `raw_bits()` and `from_raw_bits()` on packed storage.
- Type traits: `takum_floating_point` concept mirroring `std::floating_point`.
- NaR handling: `is_nar()`, `to_expected()` with `std::expected<takum<N>, takum_error>` (or `std::optional` fallback) for error propagation.
- Precision analysis: `effective_p<N>()`, `lambda_p<N>()` for error bounds (Proposition 11 analog, `[precision_traits.h](include/takum/precision_traits.h)`).
- Partial deprecations: `float8_t` shim for non-standard 8-bit float; `expected_shim` for pre-C++23 (`[compatibility.h](include/takum/compatibility.h)`).
- Bitwise operations: `~` (inversion), `reciprocal()` (bitwise `~x + 1` for division by x, Proposition 7).

## Pending Floating-Point Replacements
The following features from C++98 to C++26 (including deprecations) have not yet been fully replaced:
- Full `<cmath>`: Trigonometric/hyperbolic (`sin`, `cos`, `tan`, `asin`, etc.), exponential/logarithmic (`exp`, `log`, `log10`), power/root (`pow`, `sqrt`, `cbrt`, `hypot`), rounding/classification (`floor`, `ceil`, `round`, `fmod`, `isfinite`, `isnan`, `isinf`, `fpclassify`), constants (`pi_v<takum<N>>`).
- `<complex>`: `std::complex<takum<N>>` overloads and `complex_takum<N>` struct.
- `<random>`: Distributions like `std::uniform_real_distribution<takum<N>>`, `std::normal_distribution<takum<N>>`.
- `<numeric>`: Specific overloads for `accumulate`, `reduce`, `partial_sum` on takum containers.
- I/O and formatting: `operator<<` for `std::ostream`, `std::format("{:.2t}", takum<N>)`, `<charconv>` `to_chars`/`from_chars`.
- `<ranges>`: Views and algorithms like `iota(takum{0}, takum{10}) | transform(sin)`.
- Other integrations: `<chrono>` `duration<takum<N>>`, `std::atomic<takum<N>>`, `std::valarray<takum<N>>`, execution policies, coroutines (`co_yield takum<N>`), modules.
- Full deprecations: `<math.h>` C-style aliases (`sinf`, `logl`), reverted C++26 behaviors (e.g., `pow(NaN,0)=1` → NaR), subnormals (saturate to bounds), old NaN comparisons (`NaN != NaN`), hex literals (`0x1.2p3`), old rounding modes, `valarray` legacy ops, `bind1st` binders.
- Advanced: Functional abstractions (monads, pipelines), optimizations (SIMD/LUT), linear Takum variant, full `<stdfloat>` interop (e.g., `std::float16_t` → `takum16`).

## Post-Parity Extensions and Advanced Features

Upon achieving full feature parity with the C++ floating-point standard up to C++26, TakumCpp will be extended to incorporate a rich set of advanced mathematical constructs for numerical exploration. These features will move beyond standard arithmetic to provide a comprehensive toolkit for theoretical and applied mathematics.

### Hypercomplex Number Systems
The library will introduce a variety of hypercomplex number systems, allowing for operations in higher-dimensional algebras. The planned implementations include:
-   **Quaternions:** For 4D rotations and mechanics.
-   **Octonions:** An 8-dimensional non-associative algebra.
-   **Sedenions:** A 16-dimensional algebra.
-   **Trigintaduonions:** A 32-dimensional algebra.
-   **Cayley-Dickson Construction:** The underlying mechanism to generate higher-order algebras like octonions and sedenions.
-   **Split-complex numbers:** An alternative 2D number system with a non-real unit `j` where `j² = +1`.
-   **Multicomplex numbers:** A system built by applying the complex number construction recursively.
-   **Modified Cayley-Dickson construction:** A variation of the construction process.
-   **Dual numbers:** Used in automatic differentiation and kinematics.
-   **Generalized complex numbers:** A broader classification of 2D number systems.
-   **Clifford algebras:** A framework that generalizes complex numbers and quaternions.
-   **Quotient rings:** To formally construct number systems like complex and dual numbers.

### Hyperoperations
The library will explore the sequence of arithmetic operations beyond exponentiation, known as hyperoperations. This will include:
-   **Tetration:** Iterated exponentiation.
-   **Pentation:** Iterated tetration.
-   **Higher Hyperoperations:** Hexation, Heptation, Octation, Enneation, Decation, and generalized n-ation.
-   **Infinite hyperoperations:** Circulation (the limit of hyperoperations at infinity) and Omegation (using transfinite ordinals).
-   **Lower-order hyperoperations:** Including Zeration and the successor function.
-   **Fractional hyperoperations:** Such as Sesquation (level 1.5).
-   **Lower hyperoperations:** Including lower tetration, lower pentation, etc.
-   **Mixed hyperoperations:** Combining left-to-right and right-to-left evaluation orders.
-   **Balanced hyperoperations:** Defining operations with a balanced evaluation order.
-   **Commutative hyperoperations:** A separate hierarchy of operations that retain commutativity.

### Mathematical Constants
A comprehensive suite of mathematical constants will be provided with Takum precision.
-   **Core Constants:** Pi (π), Tau (τ), Euler's number (e), Imaginary Unit (i).
-   **Sequence-based Constants:** Reciprocal Fibonacci constant (ψ), Golden ratio (φ), Supergolden ratio (ψ), Silver ratio (δs), Plastic ratio (ρ), Tribonacci constant, Viswanath's constant, Embree-Trefethen constant, Conway's constant (λ).
-   **Geometric Constants:** Golden Angle (α), Magic Angle (θm), Pythagoras constant (√2), Polygon Circumscribing Constant (R), Kepler-Bouwkamp constant (ρ).
-   **Analysis Constants:** Euler-Mascheroni constant (γ), Meissel-Mertens constant (M), Prime constant (ρ), Kolakoski constant, Mills' constant (A), Erdős–Borwein constant (E), Brun's constant (for twin primes, B₂), Brun's constant for prime quadruplets (B₄), Niven's constant (C), Catalan's constant (G), Apéry's constant (ζ(3)), Wallis constant, Somos' quadratic recurrence constant (σ), Dottie number, Foias constant (α), Landau–Ramanujan constant (b), Cahen's constant (C).

These extensions will establish TakumCpp as a powerful and versatile library for both conventional high-performance computing and advanced numerical research.

## Requirements
- C++26 capable compiler recommended (GCC/Clang recent, MSVC 2022+); guarded fallbacks support C++23 toolchains.
- CMake 3.20+.

## Build Instructions
1. Clone the repository:
   ```
   git clone https://github.com/yourusername/TakumCpp.git
   cd TakumCpp
   ```

2. Create build directory and configure:
   ```
   mkdir build
   cd build
   cmake ..
   ```

3. Build the project:
   ```
   cmake --build .
   ```

4. Run tests (optional):
   ```
   ctest
   ```

For development builds with sanitizers, use:
```
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## Usage
Include the headers:
```cpp
#include <takum/core.h>
#include <takum/arithmetic.h>
```

Example:
```cpp
#include <takum/core.h>
#include <iostream>

int main() {
    takum<32> x = takum<32>::encode(3.14);
    takum<32> y = takum<32>::encode(2.0);
    auto sum = x + y;
    std::cout << takum<32>::decode(sum) << std::endl;
    return 0;
}
```

## Documentation
See `docs/` for specifications, coding standards, and API details.

## License
MIT License (see LICENSE).