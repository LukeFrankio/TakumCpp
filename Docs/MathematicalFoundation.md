# Mathematical Foundation of TakumCpp

This document provides the comprehensive mathematical foundation for the TakumCpp library, separated from implementation details to ensure clarity and maintainability.

## Core Mathematical Concepts

### Takum Encoding (Definition 2)

The takum encoding follows the logarithmic tapered format as defined in the reference specification:

**Definition**: For a takum with N bits (N ≥ 12), the encoding τ: ℝ → {0,1}^N maps real values to N-bit patterns using:

1. **Sign bit (S)**: Most significant bit indicating positive (0) or negative (1)
2. **Regime field (R)**: Variable-length field encoding the magnitude scale  
3. **Exponent field (E)**: Fixed-width field for fine-grained scaling
4. **Mantissa field (M)**: Remaining bits for fractional precision

### Logarithmic Space Representation (ℓ-space)

The core innovation of takum arithmetic is the ℓ-space representation:

**ℓ = 2 · log(|value|)**

This logarithmic representation enables:
- Exact multiplication: ℓ_result = ℓ_a + ℓ_b
- Exact division: ℓ_result = ℓ_a - ℓ_b  
- Simplified addition/subtraction via Gaussian-log functions

### Gaussian-log Functions (Φ)

Addition and subtraction in takum arithmetic use the Gaussian-log helper functions:

**Φ^+(t) = log(1 + e^t)**
**Φ^-(t) = log(1 - e^t)**

These functions enable accurate addition:
**log(a + b) = log(a) + Φ^+(log(b) - log(a))**

### Propositions and Properties

#### Proposition 3 (Uniqueness)
Every takum bit pattern corresponds to exactly one real value, ensuring bijective mapping within the representable range.

#### Proposition 4 (Monotonicity)  
The takum encoding preserves ordering: if x < y in real arithmetic, then τ(x) < τ(y) in two's complement bit ordering.

#### Proposition 11 (Accuracy Bounds)
The relative error for takum arithmetic operations is bounded by λ(p) < (2/3)ε(p), where ε(p) is the machine epsilon for precision p.

## NaR (Not-a-Real) Handling

### Definition 7 (Total Ordering)
NaR represents undefined results and follows total ordering semantics:
- NaR is the smallest value in the ordering
- NaR == NaR evaluates to true
- Any arithmetic operation with NaR operands produces NaR

### NaR Propagation Rules
1. **Arithmetic**: Any operation with NaR input produces NaR output
2. **Comparisons**: NaR compares less than any finite value
3. **Special cases**: 0/0, ∞-∞, and other undefined operations yield NaR

## Phase-4 Φ Evaluation Strategies

### Strategy Selection by Precision

**takum16 and takum32**: Lookup Table + Interpolation
- LUT size: 1024 entries for takum16, 4096 entries for takum32
- Interpolation: Linear for basic accuracy, cubic for enhanced precision
- Storage: Fixed-point format (Q16) for cache efficiency

**takum64**: Hybrid Approach
- Coarse LUT: 256 entries for initial approximation
- Polynomial refinement: Degree-5 to degree-7 minimax polynomials
- Coefficients: Pre-generated using offline optimization

**takum128+**: Pure Polynomial
- High-degree minimax polynomials (degree-7 to degree-9)
- Coefficient precision: Extended precision arithmetic
- Domain partitioning: Multiple polynomial pieces for accuracy

### Accuracy Requirements

Each strategy must satisfy the accuracy bound λ(p):

| Precision | λ(p) Bound | Implementation Strategy |
|-----------|------------|------------------------|
| takum16   | < 1.0e-3   | LUT + Linear Interpolation |
| takum32   | < 1.0e-6   | LUT + Cubic Interpolation |
| takum64   | < 1.0e-12  | Hybrid (LUT + Polynomial) |
| takum128  | < 1.0e-24  | High-degree Polynomial |

## Saturation and Overflow Handling

### Algorithm 1 (Saturation)
When arithmetic results exceed the representable range:

1. **Positive overflow**: Saturate to largest positive takum value
2. **Negative overflow**: Saturate to largest negative takum value  
3. **Underflow**: Round to zero (signed appropriately)
4. **Invalid operations**: Return NaR

### Dynamic Range
Takum provides symmetric dynamic range around unity:
- **Range**: [√e^{-255}, √e^{255}] for typical implementations
- **Base**: √e ≈ 1.6487 provides optimal tapering properties
- **Precision distribution**: Higher relative precision near unity

## Error Analysis and Bounds

### Rounding Error Accumulation
The modular arithmetic design minimizes rounding error through:

1. **Single rounding**: Each operation performs at most one rounding step
2. **ℓ-space precision**: Extended precision in logarithmic domain
3. **Φ accuracy**: Precomputed function values reduce cumulative error

### Comparative Accuracy
Takum provides superior accuracy compared to posits for:
- Multiplication and division (exact in ℓ-space)
- Transcendental functions (native logarithmic domain)
- Mixed-precision arithmetic (unified encoding)

## Implementation Considerations

### Bit-level Operations
- **Sign extraction**: Most significant bit
- **Two's complement**: Natural ordering for comparisons
- **Bitwise negation**: Efficient sign flipping via XOR

### Cache Efficiency
- **Packed storage**: Optimal memory layout for SIMD
- **LUT organization**: Cache-friendly access patterns
- **Coefficient storage**: Minimal memory footprint

### Numerical Stability
- **Magnitude ordering**: Larger operand processed first
- **Cancellation detection**: Special handling for near-zero results
- **Domain clamping**: Input validation for Φ functions

This mathematical foundation ensures that TakumCpp implementations are both theoretically sound and practically efficient.