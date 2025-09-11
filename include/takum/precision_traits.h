/**
 * @file precision_traits.h
 * @brief Precision analysis and error budget management for takum<N> arithmetic.
 *
 * This header provides constexpr utilities for analyzing precision characteristics
 * and managing error budgets in takum arithmetic operations. The implementation
 * follows the theoretical framework from Proposition 11 analogues and provides
 * conservative bounds suitable for Φ-based addition algorithms in the current
 * takum implementation.
 *
 * @deprecated The term "Phase-4 Φ-based addition algorithms" is deprecated.
 * This refers to the current takum arithmetic implementation.
 *
 * @details
 * The precision model accounts for:
 * - Structural bit overhead (sign, discriminator, regime encoding)
 * - Effective mantissa precision p
 * - λ(p) error bounds for floating-point operations
 * - Combined error propagation through operation sequences
 *
 * **Theoretical Background:**
 * The λ(p) function provides a conservative upper bound on the relative error
 * introduced by a single floating-point operation with p bits of precision.
 * This bound is used to budget approximation and rounding errors in complex
 * arithmetic sequences.
 *
 * @note This is intentionally lightweight; more sophisticated traits tables
 *       can extend this foundation for specialized use cases.
 *
 * @see takum::precision::effective_p for mantissa bit calculations
 * @see takum::precision::lambda_p for error bound estimation  
 */

#pragma once

#include <cstddef>
#include <cmath>

/**
 * @namespace takum::precision
 * @brief Precision analysis and error budget utilities for takum arithmetic.
 *
 * This namespace contains compile-time utilities for analyzing the precision
 * characteristics of takum<N> types and managing error budgets in arithmetic
 * operations. All functions are constexpr and have zero runtime overhead.
 */
namespace takum::precision {

/**
 * @brief Calculate effective mantissa precision for takum<N>.
 *
 * Estimates the number of effective mantissa bits available for a given
 * takum width N, accounting for structural overhead from sign, discriminator,
 * and regime encoding bits.
 *
 * @tparam N Total bit width of the takum format  
 * @return Estimated effective mantissa precision in bits
 *
 * @details
 * **Conservative Model:**
 * - Reserves 5 structural bits (sign + discriminator + regime minimum)
 * - For N > 12: reserves up to 12 bits total for structure (sign + regime)
 * - For smaller N: uses more conservative structural overhead
 * - Minimum return value is 1 bit (safety floor)
 *
 * **Examples:**
 * - takum<32>: ~20 effective mantissa bits  
 * - takum<64>: ~52 effective mantissa bits
 * - takum<16>: ~4 effective mantissa bits
 *
 * @note This is a conservative approximation; actual precision may vary
 *       based on magnitude range and specific value encoding
 */
template <size_t N>
constexpr size_t effective_p() noexcept {
  return (N > 12) ? (N - 12) : (N > 5 ? N - 5 : 1); // fallback floor
}

/**
 * @brief Calculate λ(p) error bound for the given precision.
 *
 * Computes a conservative upper bound on the relative error introduced by
 * floating-point operations with p bits of mantissa precision. This bound
 * is used for error budget analysis in multi-step arithmetic sequences.
 *
 * @tparam N Total bit width of the takum format
 * @return λ(p) error bound as a long double value
 *
 * @details
 * **Error Model:**
 * - Uses coarse approximation: λ(p) ≈ 0.66 × 2^(-p)
 * - Factor 0.66 accounts for rounding behavior (≈ 2/3 ULP)
 * - Conservative bound suitable for worst-case error analysis
 * - Based on effective precision from effective_p<N>()
 *
 * **Theoretical Justification:**
 * The λ(p) bound derives from classical floating-point error analysis
 * and provides a safe upper bound for use in error propagation formulas.
 * The factor 0.66 accounts for typical rounding behavior in IEEE-like
 * arithmetic systems.
 *
 * @see effective_p<N>() for precision calculation details
 */
template <size_t N>
constexpr long double lambda_p() noexcept {
  constexpr size_t p = effective_p<N>();
  return 0.66L * ldexpl(1.0L, -static_cast<int>(p));
}

/**
 * @brief Combine multiple error sources with optional rounding error.
 *
 * Computes the total accumulated error from multiple independent error sources,
 * optionally including a final rounding step. Used for error budget analysis
 * in complex arithmetic operation sequences.
 *
 * @tparam N Total bit width of the takum format (for budget comparison)
 * @param a First absolute error source
 * @param b Second absolute error source  
 * @param rounding Optional rounding error (default: 0.0)
 * @return Combined total absolute error
 *
 * @details
 * **Error Combination Model:**
 * - Simple summation: total = a + b + rounding
 * - Assumes independent, worst-case error sources
 * - Conservative approach suitable for safety-critical applications
 * - Caller should compare result against lambda_p<N>() budget
 *
 * **Usage Pattern:**
 * ```cpp
 * long double total_error = combined_error<32>(approx_error, interp_error, ulp/2);
 * if (total_error <= lambda_p<32>()) {
 *     // Error budget satisfied
 * }
 * ```
 *
 * @note More sophisticated error models could use quadrature sum for
 *       independent random errors, but linear sum is more conservative
 */
template <size_t N>
constexpr long double combined_error(long double a, long double b, long double rounding = 0.0L) noexcept {
  long double total = a + b + rounding;
  long double budget = lambda_p<N>();
  return (total > budget) ? total : total; // caller compares vs budget explicitly
}

} // namespace takum::precision
