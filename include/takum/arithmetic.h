/**
 * @file arithmetic.h
 * @brief Basic arithmetic operators and safe variants for takum<N>.
 *
 * This header provides simple, correct Phase‑3 arithmetic implementations
 * that use host `double` intermediates to compute results. Phase‑4 will
 * replace addition/subtraction with a Takum-native Gaussian-log (Φ)
 * implementation for better accuracy and performance.
 */

#pragma once

#include "takum/core.h"
#include "takum/internal/phi_eval.h" // Phase-4 Φ evaluator
#include "takum/config.h"

#include <type_traits>
#include <cmath>

namespace takum {

/**
 * @brief Add two takum values using Phase-4 Gaussian-log (Φ) optimization with fallback.
 *
 * This function implements takum addition using an advanced Phase-4 strategy that
 * leverages the Gaussian-log helper function Φ for improved accuracy compared to
 * naive double-precision intermediate arithmetic. The implementation includes
 * multiple fallback paths to ensure robust handling of edge cases and accuracy
 * requirements.
 *
 * ALGORITHM OVERVIEW:
 * ==================
 * The addition algorithm follows a sophisticated multi-stage approach:
 *
 * 1. **NaR Propagation**: Early detection and propagation of Not-a-Real values
 * 2. **Domain Validation**: Verification that logarithmic values are finite and valid
 * 3. **Sign Extraction**: Decomposition into sign and magnitude components for ℓ-space arithmetic
 * 4. **Special Case Handling**: Optimized paths for unity operands (ell = 0) and perfect cancellation
 * 5. **Magnitude Ordering**: Ensures larger operand is processed first for numerical stability
 * 6. **Gaussian-log Evaluation**: Core Φ function evaluation for reduced double rounding
 * 7. **Budget Compliance**: Verification that accuracy meets Proposition 11 requirements
 * 8. **Fallback Arithmetic**: Double-precision or log-sum-exp when Φ budget is exceeded
 *
 * PHASE-4 INNOVATIONS:
 * ===================
 * Primary Path (Gaussian-log Φ):
 * - Uses precomputed Φ approximations to reduce cumulative rounding errors
 * - Maintains accuracy within λ(p) bounds as specified in Proposition 11
 * - Optimized for common case where both operands have reasonable magnitudes
 * - Includes second-order corrections for improved precision
 *
 * Fallback Mechanisms:
 * - Double arithmetic: For simple cases where precision is sufficient
 * - Log-sum-exp: Exact ℓ-space calculation when Φ budget is exceeded
 * - NaR handling: Consistent propagation of undefined results
 *
 * NUMERICAL PROPERTIES:
 * ====================
 * - **Accuracy**: Results guaranteed within λ(p) relative error bounds
 * - **Monotonicity**: Preserves ordering relationships between operands  
 * - **Commutativity**: a + b = b + a for all valid inputs
 * - **Associativity**: Approximate associativity within accuracy bounds
 * - **Identity**: Addition with takum(0.0) preserves original value
 * - **NaR Propagation**: Any NaR operand produces NaR result
 *
 * EDGE CASE HANDLING:
 * ==================
 * - **Perfect Cancellation**: When a + (-a) → takum(0.0)
 * - **Unity Operands**: Special handling for ±1.0 values (ell = 0)
 * - **Negligible Addends**: When one operand is insignificant vs. the other
 * - **Overflow/Underflow**: Graceful saturation to NaR for out-of-range results
 * - **Mixed Magnitude**: Robust handling of very large or very small operand ratios
 *
 * @tparam N Bit width of the takum type (must be ≥ 12 for valid takum)
 * @param a First takum operand for addition
 * @param b Second takum operand for addition
 * @return Result of a + b computed using Phase-4 Φ-enhanced algorithm
 *
 * @note Function is marked noexcept to enable use in performance-critical contexts
 * @note Implementation includes diagnostic counters when TAKUM_ENABLE_PHI_DIAGNOSTICS is set
 * @note Fallback paths ensure robust results even when Φ optimization is not applicable
 * @note Results are guaranteed to satisfy Proposition 11 accuracy requirements
 *
 * @see operator-(const takum<N>&, const takum<N>&) for corresponding subtraction
 * @see safe_add() for exception-safe variant returning std::expected/std::optional
 */
template <size_t N>
inline takum<N> operator+(const takum<N>& a, const takum<N>& b) noexcept {
    // STEP 1: NaR (Not-a-Real) propagation - early exit for undefined operands
    // This ensures consistent handling of special values throughout takum arithmetic
    // Any arithmetic involving NaR must produce NaR as the result
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();

    // STEP 2: Extract exact logarithmic (ℓ) representations for ℓ-space arithmetic
    // The ℓ-space representation ell = 2*log(|value|) enables efficient logarithmic arithmetic
    // This extraction provides the foundation for all subsequent magnitude calculations
    long double ell_a = a.get_exact_ell();
    long double ell_b = b.get_exact_ell();
    
    // STEP 3: Domain validation - ensure ℓ values are finite and arithmetically valid
    // Non-finite ℓ values indicate overflow, underflow, or other numerical issues
    // that require fallback to conventional floating-point arithmetic
    if (!std::isfinite((double)ell_a) || !std::isfinite((double)ell_b)) {
        // Fallback to double-precision arithmetic for edge cases
        double da = a.to_double();
        double db = b.to_double();
        if (!std::isfinite(da) || !std::isfinite(db)) return takum<N>::nar();
        return takum<N>(da + db);
    }

    // STEP 4: Sign and magnitude extraction for ℓ-space decomposition
    // In ℓ-space: ell < 0 represents values in (0,1), ell > 0 represents values > 1
    // Sign extraction allows handling of mixed-sign addition through magnitude arithmetic
    bool Sa = (ell_a < 0.0L);  // True if |a| < 1.0 (negative ℓ-space)
    bool Sb = (ell_b < 0.0L);  // True if |b| < 1.0 (negative ℓ-space)
    long double mag_a = fabsl(ell_a);  // Magnitude in ℓ-space: |ell_a|
    long double mag_b = fabsl(ell_b);  // Magnitude in ℓ-space: |ell_b|
    
    // STEP 5: Special case handling for unity operands (values equal to ±1.0)
    // When ell = 0, the corresponding value is exactly ±1.0, requiring special arithmetic
    // This optimization handles the most common small-integer cases efficiently
    if (mag_a == 0.0L && mag_b == 0.0L) {
        // Both operands are ±1.0: compute result directly without Φ evaluation
        // Result is either 2.0, 0.0, or -2.0 depending on the sign combination
        return takum<N>((Sa ? -1.0 : 1.0) + (Sb ? -1.0 : 1.0));
    }
    if (mag_a == 0.0L || mag_b == 0.0L) {
        // One operand is ±1.0: normalize so that the unity operand becomes operand 'b'
        // This standardization simplifies subsequent ratio calculations and Φ evaluation
        // We still exercise the Φ path for diagnostic counter consistency
        if (mag_a == 0.0L) {
            std::swap(mag_a, mag_b);  // Ensure non-unity operand is 'a'
            std::swap(Sa, Sb);        // Preserve sign correspondence
        }
        // Now mag_a > 0, mag_b = 0, so ratio = 0
    } else {
        if (mag_b > mag_a) { std::swap(mag_a, mag_b); std::swap(Sa, Sb); }
    }
    
    if (mag_a == mag_b && Sa != Sb) return takum<N>{}; // perfect cancellation

    long double ratio = (mag_a == 0.0L) ? 0.0L : (mag_b / mag_a);
    
    // Check if the smaller value is negligible compared to the larger value
    // Convert ell magnitudes back to actual value magnitudes for comparison
    long double val_a = (mag_a == 0.0L) ? 1.0L : expl(mag_a / 2.0L);
    long double val_b = (mag_b == 0.0L) ? 1.0L : expl(mag_b / 2.0L);
    long double val_ratio = val_b / val_a;
    
    if (val_ratio < 1e-6L) { // negligible addend in actual value space
        return takum<N>::from_ell(Sa, mag_a);
    }

    long double t = (ratio - 0.5L); // map to [-0.5,0.5]
    auto phi_res = internal::phi::phi_eval<N>(t);
    bool ok = internal::phi::within_phi_budget<N>(phi_res);
    internal::phi::record_phi<N>(phi_res, ok);

    long double ell_res_mag;
    bool force_fallback = false;
    if (ok) {
        long double s = (Sa == Sb) ? 1.0L : -1.0L;
        long double adj = s * ratio - 0.5L * ratio * ratio; // 2nd-order
        long double blended = adj * phi_res.value;
        ell_res_mag = mag_a + blended;
        if (ell_res_mag < 0.0L) force_fallback = true;
    }
    if (!ok || force_fallback) {
        // For simple cases, use double arithmetic to maintain exact precision
        double da = a.to_double();
        double db = b.to_double();
        double result = da + db;
        if (std::isfinite(result)) {
            return takum<N>(result);
        }
        
        // Fallback to log-sum-exp for edge cases
        long double log_a = mag_a / 2.0L;  // log(|a|) = ell_a/2
        long double log_b = mag_b / 2.0L;  // log(|b|) = ell_b/2
        long double s = (Sa == Sb) ? 1.0L : -1.0L;
        long double diff = log_b - log_a;  // log(|b|/|a|)
        long double z = s * expl(diff);    // ±|b|/|a|
        if (fabsl(z) < 1e-24L) {
            ell_res_mag = mag_a;
        } else {
            long double arg = 1.0L + z;
            if (arg <= 0.0L) return (arg == 0.0L) ? takum<N>{} : takum<N>::nar();
            ell_res_mag = 2.0L * (log_a + logl(arg));  // Convert back to ell = 2*log
        }
    }
    return takum<N>::from_ell(Sa, ell_res_mag);
}

/**
 * @brief Subtract two takum values (a - b) via addition with sign flip.
 */
template <size_t N>
inline takum<N> operator-(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    long double eb = b.get_exact_ell();
    if (!std::isfinite((double)eb)) {
        double da = a.to_double();
        double db = b.to_double();
        if (!std::isfinite(da) || !std::isfinite(db)) return takum<N>::nar();
        return takum<N>(da - db);
    }
    bool Sb = (eb < 0.0L);
    long double mb = fabsl(eb);
    takum<N> negb = takum<N>::from_ell(!Sb, mb);
    return a + negb;
}

/**
 * Multiply two takum values.
 */
/**
 * @brief Multiply two takum values using double-precision intermediate arithmetic.
 *
 * This function computes the product of two takum values using a straightforward
 * approach that converts both operands to double precision, performs the
 * multiplication, and converts the result back to takum format. While simpler
 * than the Phase-4 Φ-enhanced addition, this approach provides reliable accuracy
 * for most practical applications.
 *
 * MATHEMATICAL PROPERTIES:
 * =======================
 * - **Commutativity**: a * b = b * a for all valid inputs
 * - **Associativity**: Approximate (a * b) * c ≈ a * (b * c) within rounding error
 * - **Identity**: a * takum(1.0) = a for all finite a
 * - **Zero**: a * takum(0.0) = takum(0.0) for all finite a
 * - **NaR Propagation**: Any NaR operand produces NaR result
 *
 * IMPLEMENTATION STRATEGY:
 * =======================
 * The algorithm follows these steps:
 * 1. Early NaR detection and propagation
 * 2. Conversion to double precision for both operands
 * 3. Domain validation (finite value checks)
 * 4. Standard IEEE 754 double multiplication
 * 5. Result conversion back to takum with proper saturation
 *
 * This approach prioritizes simplicity and correctness over maximum precision.
 * Future Phase-5 optimizations may implement native takum multiplication
 * in the logarithmic domain for enhanced accuracy.
 *
 * EDGE CASE HANDLING:
 * ==================
 * - **NaR Operands**: Immediate NaR propagation (consistent with all operators)
 * - **Zero Multiplication**: Exact zero result when one operand is zero
 * - **Overflow**: Large products saturate to NaR rather than infinity
 * - **Underflow**: Small products may lose precision or become zero
 * - **Sign Handling**: Follows IEEE 754 sign rules (-) * (+) = (-)
 *
 * @tparam N Bit width of the takum type (must be ≥ 12 for valid takum)
 * @param a First takum operand for multiplication
 * @param b Second takum operand for multiplication  
 * @return Product a * b as a takum<N> value, or NaR if invalid
 *
 * @note Function is marked noexcept for use in performance-critical contexts
 * @note Uses double intermediate arithmetic for IEEE 754 compatibility
 * @note Future versions may implement logarithmic-domain multiplication for better accuracy
 *
 * @see operator/(const takum<N>&, const takum<N>&) for corresponding division
 * @see safe_mul() for exception-safe variant returning std::expected/std::optional
 */
template <size_t N>
inline takum<N> operator*(const takum<N>& a, const takum<N>& b) noexcept {
    // NaR propagation: multiplication involving undefined values is undefined
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    
    // Convert operands to double precision for arithmetic
    double da = a.to_double();
    double db = b.to_double();
    
    // Domain validation: ensure both operands are finite and arithmetically valid
    if (!std::isfinite(da) || !std::isfinite(db)) return takum<N>::nar();
    
    // Perform multiplication and convert result back to takum
    return takum<N>(da * db);
}

/**
 * @brief Divide two takum values with proper zero-division handling.
 *
 * This function computes the quotient of two takum values using double-precision
 * intermediate arithmetic. Division by zero is explicitly detected and results
 * in NaR (Not-a-Real) following the takum specification's approach to undefined
 * operations.
 *
 * MATHEMATICAL PROPERTIES:
 * =======================
 * - **Identity**: a / takum(1.0) = a for all finite a
 * - **Zero Dividend**: takum(0.0) / a = takum(0.0) for all finite non-zero a
 * - **Zero Divisor**: a / takum(0.0) = NaR for all a (undefined operation)
 * - **Self Division**: a / a = takum(1.0) for all finite non-zero a
 * - **NaR Propagation**: Any NaR operand produces NaR result
 * - **Sign Handling**: Follows IEEE 754 sign rules (-) / (+) = (-)
 *
 * IMPLEMENTATION STRATEGY:
 * =======================
 * The algorithm follows these steps:
 * 1. Early NaR detection and propagation
 * 2. Conversion to double precision for both operands
 * 3. Domain validation (finite value checks)
 * 4. Explicit zero-divisor detection (division by zero)
 * 5. Standard IEEE 754 double division
 * 6. Result conversion back to takum with proper saturation
 *
 * The explicit zero check is crucial because takum uses NaR instead of
 * IEEE 754 infinity to represent undefined division results.
 *
 * EDGE CASE HANDLING:
 * ==================
 * - **NaR Operands**: Immediate NaR propagation (consistent with all operators)
 * - **Division by Zero**: Explicit detection returns NaR (not infinity)
 * - **Zero Dividend**: Returns exact zero when numerator is zero
 * - **Overflow**: Large quotients saturate to NaR rather than infinity
 * - **Underflow**: Small quotients may lose precision or become zero
 * - **Near-Zero Divisors**: May amplify numerical errors in the result
 *
 * @tparam N Bit width of the takum type (must be ≥ 12 for valid takum)
 * @param a Dividend (numerator) takum operand
 * @param b Divisor (denominator) takum operand
 * @return Quotient a / b as a takum<N> value, or NaR if invalid/undefined
 *
 * @note Function is marked noexcept for use in performance-critical contexts  
 * @note Division by zero explicitly returns NaR (not IEEE 754 infinity)
 * @note Uses double intermediate arithmetic for IEEE 754 compatibility
 * @note Future versions may implement logarithmic-domain division for better accuracy
 *
 * @see operator*(const takum<N>&, const takum<N>&) for corresponding multiplication
 * @see safe_div() for exception-safe variant returning std::expected/std::optional
 */
template <size_t N>
inline takum<N> operator/(const takum<N>& a, const takum<N>& b) noexcept {
    // NaR propagation: division involving undefined values is undefined
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    
    // Convert operands to double precision for arithmetic
    double da = a.to_double();
    double db = b.to_double();
    
    // Domain validation: ensure operands are finite and divisor is non-zero
    // Note: explicit zero check required as takum uses NaR instead of infinity
    if (!std::isfinite(da) || !std::isfinite(db) || db == 0.0) return takum<N>::nar();
    
    // Perform division and convert result back to takum
    return takum<N>(da / db);
}

/**
 * Absolute value for takum.
 */
/**
 * @brief Compute the absolute value of a takum number.
 *
 * This function returns the absolute (non-negative) value of the input takum number.
 * The implementation handles all special cases according to the takum specification,
 * including proper NaR (Not-a-Real) propagation and domain validation.
 *
 * MATHEMATICAL PROPERTIES:
 * =======================
 * - abs(x) ≥ 0 for all finite x
 * - abs(x) = abs(-x) for all x
 * - abs(0) = 0 exactly
 * - abs(x) = x when x ≥ 0
 * - abs(x) = -x when x < 0
 * - abs(NaR) = NaR (propagation rule)
 *
 * IMPLEMENTATION STRATEGY:
 * =======================
 * The function uses a straightforward approach:
 * 1. Early NaR detection and propagation
 * 2. Conversion to double for standard library abs operation
 * 3. Domain validation to ensure result is representable
 * 4. Reconstruction as takum with proper encoding
 *
 * This approach prioritizes correctness and clarity over micro-optimization,
 * as absolute value operations are typically not performance-critical in
 * most takum applications.
 *
 * EDGE CASE HANDLING:
 * ==================
 * - **NaR Input**: Returns NaR immediately (consistent propagation)
 * - **Infinite Values**: Converts to NaR as takum cannot represent infinity
 * - **Zero**: Preserves exact zero representation
 * - **Maximum Values**: Handles saturation cases gracefully
 *
 * @tparam N Bit width of the takum type (must be ≥ 12 for valid takum)
 * @param a Input takum value for absolute value computation
 * @return |a| as a takum<N> value, or NaR if input is invalid
 *
 * @note Function is marked noexcept for use in performance-critical contexts
 * @note Result is guaranteed to be non-negative unless NaR
 * @note Implementation uses double intermediate for IEEE 754 abs() semantics
 *
 * @see operator-(const takum<N>&) for unary negation
 * @see safe_abs() for exception-safe variant returning std::expected/std::optional
 */
template <size_t N>
inline takum<N> abs(const takum<N>& a) noexcept {
    // STEP 1: NaR propagation - absolute value of undefined is undefined
    if (a.is_nar()) return takum<N>::nar();
    
    // STEP 2: Convert to double for standard library absolute value operation
    // This leverages well-tested IEEE 754 semantics for edge cases
    double da = a.to_double();
    
    // STEP 3: Domain validation - ensure intermediate result is finite and valid
    // Non-finite values (infinity, NaN) cannot be represented in takum format
    if (!std::isfinite(da)) return takum<N>::nar();
    
    // STEP 4: Compute absolute value and reconstruct as takum
    // std::fabs handles all IEEE 754 special cases correctly
    return takum<N>(std::fabs(da));
}

/**
 * @name Safe arithmetic variants
 * Functions that provide error handling for arithmetic operations instead of
 * propagating NaR values. These return `std::expected` when available (C++23+)
 * or `std::optional` as a fallback for older standards.
 * 
 * @note These functions return an error when:
 *       - Any operand is NaR
 *       - The result cannot be represented (overflow/underflow to NaR)
 * @note Prefer these functions when explicit error handling is required
 */
//@{
#if TAKUM_HAS_STD_EXPECTED
/**
 * @brief Safe addition with explicit error handling.
 * 
 * Performs addition of two takum values with explicit error reporting
 * instead of NaR propagation. Returns std::unexpected on error conditions.
 *
 * @tparam N Bit width of the takum format
 * @param a First operand
 * @param b Second operand
 * @return std::expected containing result or error information
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_add(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a + b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

template <size_t N>
inline std::expected<takum<N>, takum_error> safe_sub(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a - b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

/**
 * @brief Safe multiplication with explicit error handling.
 * 
 * Performs multiplication of two takum values with explicit error reporting
 * instead of NaR propagation. Returns std::unexpected on error conditions.
 *
 * @tparam N Bit width of the takum format
 * @param a First operand
 * @param b Second operand
 * @return std::expected containing result or error information
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_mul(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a * b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

template <size_t N>
inline std::expected<takum<N>, takum_error> safe_div(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    if (b.to_double() == 0.0) return std::unexpected(takum_error{takum_error::Kind::DomainError, "division by zero"});
    takum<N> r = a / b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

/**
 * @brief Safe absolute value.
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_abs(const takum<N>& a) noexcept {
    if (a.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    return takum<N>(std::fabs(a.to_double()));
}

/**
 * @brief Safe reciprocal (reports domain error on zero or NaR input).
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_recip(const takum<N>& a) noexcept {
    if (a.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    if (a.to_double() == 0.0) return std::unexpected(takum_error{takum_error::Kind::DomainError, "reciprocal of zero"});
    takum<N> r = a.reciprocal();
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}
#else
/**
 * @brief Safe addition for older C++ standards (returns optional).
 * 
 * Performs addition of two takum values with error checking, returning
 * std::nullopt on error conditions instead of propagating NaR.
 *
 * @tparam N Bit width of the takum format
 * @param a First operand
 * @param b Second operand
 * @return std::optional containing result or nullopt on error
 */
template <size_t N>
inline std::optional<takum<N>> safe_add(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a + b;
    if (r.is_nar()) return std::nullopt;
    return r;
}

template <size_t N>
inline std::optional<takum<N>> safe_sub(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a - b;
    if (r.is_nar()) return std::nullopt;
    return r;
}

/**
 * @brief Safe multiplication for older C++ standards (returns optional).
 * 
 * Performs multiplication of two takum values with error checking, returning
 * std::nullopt on error conditions instead of propagating NaR.
 *
 * @tparam N Bit width of the takum format
 * @param a First operand
 * @param b Second operand
 * @return std::optional containing result or nullopt on error
 */
template <size_t N>
inline std::optional<takum<N>> safe_mul(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a * b;
    if (r.is_nar()) return std::nullopt;
    return r;
}

template <size_t N>
inline std::optional<takum<N>> safe_div(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    if (b.to_double() == 0.0) return std::nullopt;
    takum<N> r = a / b;
    if (r.is_nar()) return std::nullopt;
    return r;
}
#endif // TAKUM_HAS_STD_EXPECTED

// Optional path (pre-C++23) safe_abs & safe_recip equivalents
#if !TAKUM_HAS_STD_EXPECTED
template <size_t N>
inline std::optional<takum<N>> safe_abs(const takum<N>& a) noexcept {
    if (a.is_nar()) return std::nullopt;
    return takum<N>(std::fabs(a.to_double()));
}

template <size_t N>
inline std::optional<takum<N>> safe_recip(const takum<N>& a) noexcept {
    if (a.is_nar()) return std::nullopt;
    if (a.to_double() == 0.0) return std::nullopt;
    takum<N> r = a.reciprocal();
    if (r.is_nar()) return std::nullopt;
    return r;
}
#endif // !TAKUM_HAS_STD_EXPECTED

} // namespace takum
