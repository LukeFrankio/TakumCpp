# Mathematical Functions for TakumCpp

This document describes the mathematical function library for TakumCpp, implemented as part of Phase 4 of the development plan. The library provides comprehensive mathematical operations that leverage the logarithmic nature of takum arithmetic for improved accuracy and performance.

## Overview

The mathematical functions in TakumCpp are designed to:

- Provide `<cmath>`-compatible interfaces for takum types
- Leverage the logarithmic representation for computational efficiency
- Maintain high accuracy within precision bounds defined by Proposition 11
- Handle domain errors gracefully through NaR propagation
- Offer safe variants with explicit error handling via `std::expected`
- Support deprecation shims for legacy C-style function calls

## Function Categories

### Trigonometric Functions

- **Basic**: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`
- **Domain handling**: Automatic NaR for out-of-domain inputs
- **Accuracy**: Within λ(p) bounds for all precision levels

```cpp
takum<64> x(0.5);
auto sine_val = sin(x);                    // Basic usage
auto safe_sine = safe_sin(x);              // Safe variant
auto angle = atan2(takum<64>(1), takum<64>(1)); // Two-argument arctangent
```

### Hyperbolic Functions

- **Functions**: `sinh`, `cosh`, `tanh`, `asinh`, `acosh`, `atanh`
- **Special handling**: Domain restrictions for inverse functions
- **Overflow protection**: NaR on computational overflow

### Exponential and Logarithmic Functions

- **Core functions**: `exp`, `log`, `log10`, `log2`, `exp2`
- **High-accuracy variants**: `log1p`, `expm1` for small argument accuracy
- **Logarithmic optimization**: Direct computation when possible using takum's ℓ representation

```cpp
takum<64> x(2.0);
auto natural_log = log(x);                 // Uses takum's logarithmic representation
auto safe_result = safe_log(x);            // With error handling
```

### Power and Root Functions

- **Power**: `pow(base, exponent)` with logarithmic computation: `exp(exponent * log(base))`
- **Roots**: `sqrt`, `cbrt` 
- **Distance**: `hypot` for Euclidean distance calculation
- **Special cases**: Handles negative bases with non-integer exponents

### Rounding and Remainder Functions

- **Rounding**: `floor`, `ceil`, `round`, `trunc`, `nearbyint`
- **Remainder**: `fmod`, `remainder`
- **IEEE compatibility**: Follows standard semantics where applicable

### Classification Functions

- **Finite testing**: `isfinite`, `isnan`, `isinf`, `isnormal`
- **Classification**: `fpclassify`, `signbit`
- **Takum-specific**: No infinity representation (always returns false for `isinf`)

## Φ (Gaussian-log) Integration

The mathematical functions integrate seamlessly with TakumCpp's Φ-based addition system:

### How It Works

1. **Compound operations** like `sin(a) + cos(b)` automatically use Φ evaluation for the addition
2. **Function composition** benefits from accurate intermediate results
3. **Series expansions** maintain precision through Φ-enhanced addition

### Performance Benefits

- **Reduced rounding errors** in multi-step calculations
- **Hardware-efficient** lookup tables and polynomial evaluation
- **Adaptive precision** based on operand magnitudes

```cpp
// This expression uses Φ internally for all additions
auto complex_result = sin(x) + cos(y) + exp(z);

// Function composition leverages Φ accuracy
auto composed = sin(exp(x + y));  // Both + and function calls optimized
```

## Accuracy Bounds (Proposition 11)

All mathematical functions maintain accuracy within the bounds defined by Proposition 11:

- **Relative error** ≤ λ(p) where λ(p) < (2/3)ε(p)
- **Precision-dependent**: Higher precision takum types achieve better accuracy
- **Validated testing**: Comprehensive test suite verifies bounds compliance

### Precision Examples

```cpp
takum<32>  x32(0.5);   // ~7 decimal digits accuracy
takum<64>  x64(0.5);   // ~15 decimal digits accuracy  
takum<128> x128(0.5);  // ~34 decimal digits accuracy

// All maintain accuracy within their respective λ(p) bounds
auto sin32 = sin(x32);
auto sin64 = sin(x64);
auto sin128 = sin(x128);
```

## Safe Variants and Error Handling

### std::expected Interface

Safe variants return `std::expected<takum<N>, takum_error>` for explicit error handling:

```cpp
auto result = safe_sqrt(x);
if (result.has_value()) {
    // Use result.value()
} else {
    // Handle result.error()
    switch (result.error().kind) {
        case takum_error::Kind::DomainError:
            // Handle domain error
            break;
        case takum_error::Kind::InvalidOperation:
            // Handle NaR input
            break;
        // ... other error types
    }
}
```

### Error Categories

- **DomainError**: Input outside function domain (e.g., `sqrt(-1)`)
- **InvalidOperation**: NaR input provided
- **Overflow**: Result too large to represent
- **Underflow**: Result too small to represent (rare in takum)

## Mathematical Constants

### Available Constants

```cpp
using namespace takum::math_constants;

auto pi = pi_v<takum<64>>;           // π
auto e = e_v<takum<64>>;             // Euler's number
auto sqrt2 = sqrt2_v<takum<64>>;     // √2
auto golden = phi_v<takum<64>>;      // Golden ratio φ
auto gamma = egamma_v<takum<64>>;    // Euler-Mascheroni constant γ
```

### Precision-Specific Constants

Constants are computed with appropriate precision for each takum type:

```cpp
// Different precisions, same mathematical constant
auto pi32 = pi_v<takum<32>>;    // 32-bit precision π
auto pi64 = pi_v<takum<64>>;    // 64-bit precision π  
auto pi128 = pi_v<takum<128>>;  // 128-bit precision π
```

## Deprecation and Migration

### Legacy C-Style Functions

Deprecated shims provide compatibility for legacy code:

```cpp
// Deprecated (generates warning):
auto result1 = sinf(1.0f);        // Maps to sin(takum<32>(1.0f))
auto result2 = sin(1.0);          // Maps to sin(takum<64>(1.0))
auto result3 = logl(1.0L);        // Maps to log(takum<128>(1.0L))

// Modern equivalent:
auto result1_new = sin(takum<32>(1.0f));
auto result2_new = sin(takum<64>(1.0));
auto result3_new = log(takum<128>(1.0L));
```

### Legacy Constants

```cpp
// Deprecated (generates warning):
auto pi_old = M_PI;               // Maps to pi_v<takum<64>>

// Modern equivalent:
auto pi_new = math_constants::pi_v<takum<64>>;
```

### Migration Strategy

1. **Compile with warnings** to identify deprecated usage
2. **Replace function calls** with explicit takum variants
3. **Update constants** to use math_constants namespace
4. **Add error handling** using safe variants where appropriate
5. **Define `TAKUM_NO_LEGACY_MATH_FUNCTIONS`** to disable shims

## Implementation Notes

### Computational Strategies

- **Range reduction** for trigonometric functions to improve accuracy
- **Taylor series** for functions in appropriate domains
- **Logarithmic computation** for power functions: `pow(x,y) = exp(y*log(x))`
- **Host fallback** for complex operations while maintaining takum semantics

### Multi-word Support

All functions support takum types larger than 64 bits:

```cpp
takum<128> large_x(1e50);
auto large_sin = sin(large_x);     // Handled correctly
auto large_pow = pow(large_x, takum<128>(0.5));  // Uses multi-word arithmetic
```

### Performance Considerations

- **LUT optimization** for frequently used ranges
- **Polynomial approximation** for high-precision requirements
- **Φ integration** reduces multiple rounding errors
- **Compile-time constants** for mathematical values

## Testing and Validation

### Test Coverage

- **Accuracy bounds**: Verified against Proposition 11 limits
- **Domain handling**: All boundary conditions tested
- **NaR propagation**: Comprehensive edge case validation
- **Safe variants**: Both success and error paths tested
- **Multi-word**: Full support for takum<128> and larger

### Benchmarking

- **IEEE comparison**: Accuracy relative to host double precision
- **Performance metrics**: Latency and throughput measurements
- **Memory usage**: LUT and polynomial coefficient overhead
- **Composition overhead**: Function chaining performance

## Future Enhancements

### Planned Improvements

- **SIMD optimization** for vector operations
- **Extended precision** polynomial coefficients
- **Specialized algorithms** for specific takum width ranges
- **Hardware acceleration** hooks for custom implementations

### Research Areas

- **Minimax approximations** for optimal polynomial coefficients
- **Adaptive algorithms** based on runtime precision analysis
- **Interval arithmetic** integration for error bound analysis
- **Machine learning** approaches for coefficient optimization

This mathematical function library establishes TakumCpp as a comprehensive replacement for IEEE floating-point arithmetic, with accuracy guarantees and computational efficiency that leverage the unique properties of the takum format.