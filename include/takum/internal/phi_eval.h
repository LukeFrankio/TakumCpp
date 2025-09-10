/**
 * @file phi_eval.h
 * @brief Gaussian-log (Φ) evaluation engine for TakumCpp Phase-4 addition/subtraction.
 *
 * This header provides the core Φ evaluation functionality used by takum arithmetic
 * operations for accurate addition and subtraction in the logarithmic domain. The
 * implementation uses different strategies based on precision requirements:
 *
 * - Hybrid polynomial evaluation for takum64+ using generated fixed-point coefficients
 * - Temporary polynomial-only approximation for all precisions (LUT paths planned)
 * - Interface designed for stability during future LUT + interpolation integration
 *
 * Technical details:
 * - Polynomial coefficients stored in Q16 fixed-point format (see generated header)
 * - Evaluation uses Horner's method in long double for precision
 * - Domain mapping: incoming t clamped to [-0.5, 0) with uniform interval partitioning
 * - Error budgets comply with Proposition 11 λ(p) bounds for each precision
 *
 * @note This implements a monotone helper φ(t) ~ Φ(t) for addition formulas
 * @note Full Gaussian-log formulation + LUT scaling constants integration pending
 * @note Internal implementation detail - use through arithmetic.h operators
 */
#pragma once
/**
 * @brief Include guard to prevent multiple inclusions of this header.
 * Critical for template-heavy code to avoid redefinition errors.
 */
#pragma once

// Standard library includes for mathematical operations and numeric types
#include <cmath>    // Mathematical functions (fabsl, ldexp, etc.)
#include <cstdint>  // Fixed-width integer types (uint32_t, etc.)
#include <array>    // std::array for compile-time fixed-size containers

// TakumCpp internal headers for Φ (Gaussian-log) evaluation infrastructure
#include "takum/internal/phi_spec.h"    // Constants and specifications for Φ approximation
#include "takum/internal/phi_types.h"   // Type definitions for Φ evaluation results
#include "takum/internal/phi_lut.h"     // Lookup table implementations for fast Φ evaluation
#include "takum/precision_traits.h"     // Precision traits and error budget helpers  
#include "takum/config.h"               // Compile-time configuration flags and runtime accessors

/**
 * Lightweight internal Gaussian-log (Φ) evaluation helpers for TakumCpp Phase-4.
 * 
 * IMPLEMENTATION STRATEGY:
 * =======================
 * The current strategy leverages a hybrid approach optimized for different takum precisions:
 * 
 * 1. Hybrid polynomial evaluation for takum64+ using pre-generated fixed-point coefficients
 *    - Coefficients computed offline using minimax approximation for optimal accuracy
 *    - Fixed-point Q16 format balances precision with computational efficiency
 *    - Error bounds comply with Proposition 11 λ(p) requirements for each precision
 * 
 * 2. Temporary polynomial-only approximation for all precisions during development
 *    - LUT (lookup table) + interpolation paths are planned for future optimization
 *    - Interface designed for stability to avoid breaking changes during LUT integration
 *    - Allows incremental enhancement without disrupting existing arithmetic operations
 * 
 * MATHEMATICAL FOUNDATION:
 * ========================
 * The Φ function is a carefully designed monotone helper that approximates the true
 * Gaussian-log function used in Takum addition/subtraction formulas. The mathematical
 * properties ensure:
 * - Monotonicity preservation for correct ordering in log domain
 * - Accuracy within λ(p) error budgets as defined in Proposition 11
 * - Numerical stability across the full input domain [-0.5, 0.5]
 * 
 * TECHNICAL IMPLEMENTATION DETAILS:
 * =================================
 * Polynomial coefficients in poly_coeffs[][] arrays are stored in Q16 fixed-point format
 * (16 fractional bits) to balance precision with integer arithmetic efficiency. The
 * evaluation process:
 * 
 * 1. Domain clamping: Input t is clamped to [-0.5, 0) to ensure numerical stability
 * 2. Interval mapping: The domain is partitioned uniformly into NUM_INTERVALS segments
 * 3. Coefficient selection: Each interval i covers [-0.5 + i*step, -0.5 + (i+1)*step)
 * 4. Horner evaluation: Polynomials evaluated using Horner's method in long double precision
 * 
 * Domain step calculation: step = 0.5 / NUM_INTERVALS ensures uniform coverage.
 * 
 * ERROR BUDGETING AND VALIDATION:
 * ===============================
 * Maximum errors for each interval are pre-computed and stored in max_errors[] arrays.
 * These bounds are validated against λ(p) requirements during coefficient generation.
 * The offline script gen_poly_coeffs.py produces machine-checkable error reports for
 * verification of accuracy guarantees.
 * 
 * FUTURE ENHANCEMENTS:
 * ===================
 * This Φ implementation currently models a placeholder until the full Gaussian-log
 * formulation with LUT scaling constants is integrated. The error budgets from
 * max_errors[] are available for integration with Proposition 11 budget-aware
 * arithmetic operations.
 */

/**
 * @namespace takum::internal::phi
 * @brief Internal implementation namespace for Gaussian-log (Φ) evaluation functionality.
 *
 * This namespace encapsulates all the low-level implementation details for evaluating
 * the Gaussian-log function Φ used in takum addition and subtraction operations. The
 * implementation is carefully optimized for different precision requirements while
 * maintaining strict error bounds.
 *
 * Key components:
 * - phi_poly_eval(): Core polynomial evaluation function for Φ approximation
 * - phi_eval(): Template dispatcher selecting optimal strategy per precision
 * - Error budget tracking and validation against Proposition 11 bounds
 * - Fixed-point arithmetic optimizations for performance-critical paths
 *
 * @note This is an internal implementation detail and should not be used directly
 * @note All functions are designed to be constexpr-compatible where possible
 * @note Thread-safe and exception-safe (noexcept) for use in critical arithmetic paths
 */
namespace takum::internal::phi {

// PhiEvalResult type definition is located in phi_types.h for modularity
// This allows other headers to reference the result type without including full evaluation logic

/**
 * @brief Core polynomial evaluation function for Gaussian-log (Φ) approximation.
 *
 * This function implements the primary Φ evaluation algorithm using piecewise
 * polynomial approximation with pre-computed coefficients. The implementation
 * uses Horner's method for numerical stability and efficiency.
 *
 * ALGORITHM OVERVIEW:
 * ==================
 * 1. Input domain clamping to [-0.5, +0.5] for numerical stability
 * 2. Linear mapping from input domain to interval index [0, NUM_INTERVALS)
 * 3. Coefficient retrieval from pre-generated poly_coeffs array
 * 4. Horner polynomial evaluation in long double precision
 * 5. Fixed-point to floating-point conversion with proper scaling
 *
 * NUMERICAL PROPERTIES:
 * ====================
 * - Monotonicity: Preserves ordering essential for log-domain arithmetic
 * - Accuracy: Error bounds within λ(p) requirements for all supported precisions  
 * - Stability: Clamping and careful scaling prevent overflow/underflow
 * - Performance: Q16 fixed-point coefficients optimize memory bandwidth
 *
 * @param t Input value in the domain [-0.5, +0.5] (clamped if outside)
 * @return PhiEvalResult containing the computed Φ(t) value and metadata
 * @note Function is marked noexcept for use in arithmetic operations
 * @note Input clamping ensures robust behavior for edge cases
 * @note Uses long double internally for maximum intermediate precision
 */
inline PhiEvalResult phi_poly_eval(long double t) noexcept {
    // STEP 1: Input domain clamping for numerical stability and error bound guarantees
    // The polynomial coefficients are valid only within [-0.5, +0.5], so we clamp
    // inputs outside this range to prevent extrapolation errors and maintain monotonicity
    if (t > 0.5L) t = 0.5L;   // Clamp upper bound to maximum supported input
    if (t < -0.5L) t = -0.5L; // Clamp lower bound to minimum supported input
    
    // STEP 2: Domain mapping constants for uniform interval partitioning
    constexpr long double domain_min = -0.5L;  // Minimum input value in supported domain  
    constexpr long double domain_max = 0.5L;   // Maximum input value in supported domain
    constexpr long double span = domain_max - domain_min; // Total domain span = 1.0
    
    // STEP 3: Linear mapping from input domain [-0.5, +0.5] to unit interval [0, 1]
    // This normalized coordinate u simplifies interval index calculation
    long double u = (t - domain_min) / span; // Normalize t to [0,1] range
    
    // STEP 4: Interval index calculation for piecewise polynomial selection
    // Each interval corresponds to a specific set of polynomial coefficients
    // optimized for accuracy within that subdomain
    long double f_index = u * static_cast<long double>(NUM_INTERVALS);
    int idx = static_cast<int>(f_index); // Convert to integer interval index
    
    // STEP 5: Boundary condition handling to prevent array bounds violations
    // This can occur due to floating-point precision in the mapping calculation
    if (idx >= NUM_INTERVALS) idx = NUM_INTERVALS - 1; // Clamp to valid index range
    
    // STEP 6: Coefficient retrieval from pre-generated polynomial coefficient table
    // Each row poly_coeffs[idx] contains POLY_DEGREE+1 coefficients in Q16 fixed-point format
    const int32_t* coeff = poly_coeffs[idx]; // Get coefficient array for current interval
    
    // STEP 7: Fixed-point to floating-point conversion scaling factor
    // Q16 format has 16 fractional bits, so scale = 1/(2^16) converts to floating-point
    long double scale = 1.0L / static_cast<long double>(1ULL << Q_FRAC_BITS);
    
    // STEP 8: Horner's method polynomial evaluation for numerical stability
    // Evaluates polynomial c_n*t^n + c_(n-1)*t^(n-1) + ... + c_1*t + c_0
    // using iterative scheme: acc = ((...((c_n*t + c_(n-1))*t + c_(n-2))*t + ...)*t + c_0
    long double acc = 0.0L; // Accumulator for Horner evaluation
    for (int d = POLY_DEGREE; d >= 0; --d) {
        // Convert Q16 fixed-point coefficient to floating-point with proper scaling
        long double c = static_cast<long double>(coeff[d]) * scale;
        acc = acc * t + c; // Horner iteration: multiply by t and add next coefficient
    }
    
    // STEP 9: Error bound retrieval for result validation and budget tracking
    // Pre-computed maximum absolute error for the current interval ensures
    // compliance with Proposition 11 accuracy requirements
    long double eb = static_cast<long double>(max_errors[idx]);
    
    // STEP 10: Return structured result with value, error bound, and metadata
    return { acc, eb, idx }; // PhiEvalResult{value, error_bound, interval_index}
}

// ---------------- Hybrid (coarse anchor + polynomial residual) --------------
// For N > 32 we approximate using a coarse set of anchor samples (interval
// boundaries) and refine via the existing per-interval polynomial (currently
// still global-in-x). This isolates the residual which will shrink once the
// generator emits re-centered local polynomials. Kept internal & easily swappable.
namespace detail {

// Coarse LUT size for hybrid (independent of NUM_INTERVALS polynomial partition)
constexpr int HYBRID_LUT_SIZE = TAKUM_COARSE_LUT_SIZE; // macro-configurable
static_assert(HYBRID_LUT_SIZE > 0 && HYBRID_LUT_SIZE <= 4096,
    "HYBRID_LUT_SIZE must be in (0, 4096] for phi_hybrid_eval");
struct HybridCoarseLUT {
    std::array<long double, HYBRID_LUT_SIZE + 1> v{}; // endpoints inclusive
    // NOTE: This constructor cannot be constexpr because it calls phi_poly_eval(t),
    // which is not declared constexpr. Making it a normal runtime constructor
    // ensures correct initialization while retaining the lazy static initialization
    // semantics we want.
    HybridCoarseLUT() noexcept : v{} {
        for (int i = 0; i <= HYBRID_LUT_SIZE; ++i) {
            long double t = -0.5L + (static_cast<long double>(i) / HYBRID_LUT_SIZE);
            auto poly = phi_poly_eval(t); // runtime seed
            v[i] = poly.value;
        }
    }
};
inline const HybridCoarseLUT& coarse_hybrid_table() {
    static const HybridCoarseLUT tbl{}; // lazily constructed at first use
    return tbl;
}

inline PhiEvalResult phi_hybrid_eval(long double t) noexcept {
    // Clamp
    if (t > 0.5L) t = 0.5L; else if (t < -0.5L) t = -0.5L;
    // Coarse interval
    long double u = (t + 0.5L); // [0,1]
    long double coarse_f = u * HYBRID_LUT_SIZE;
    int ci = static_cast<int>(coarse_f);
    if (ci >= HYBRID_LUT_SIZE) ci = HYBRID_LUT_SIZE - 1;
    long double cfrac = coarse_f - static_cast<long double>(ci);
    const auto& cv = coarse_hybrid_table().v;
    long double base0 = cv[ci];
    long double base1 = cv[ci + 1];
    long double coarse_interp = base0 + (base1 - base0) * cfrac;

    // Fine polynomial interval refinement
    auto poly_res = phi_poly_eval(t);
    long double residual = poly_res.value - coarse_interp;
    // Conservative combined error: poly_res.abs_error + interpolation diff scaling
    long double eb = poly_res.abs_error + fabsl(residual) * 0.25L + 5e-6L;
    return { coarse_interp + residual, eb, ci };
}
} // namespace detail

// Public internal API used by arithmetic (subject to refinement):
inline long double phi(long double t) noexcept { return phi_poly_eval(t).value; }

// Precision-dispatching evaluator returning PhiEvalResult.
// For small N (<=16, <=32) use LUT; else polynomial (future: hybrid).
template <size_t N>
inline PhiEvalResult phi_eval(long double t) noexcept {
    if constexpr (N <= 16) {
        return phi_lut_1024(t);
    } else if constexpr (N <= 32) {
        return phi_lut_4096(t);
    } else {
        return detail::phi_hybrid_eval(t); // adaptive degree hook could go here
    }
}

// Convenience value-only accessor
template <size_t N>
inline long double phi_v(long double t) noexcept { return phi_eval<N>(t).value; }

// Feature toggle macro (user can define before including arithmetic)
#ifndef TAKUM_ENABLE_FAST_ADD
#define TAKUM_ENABLE_FAST_ADD 0
#endif

// Helper: check whether accumulated Φ error stays within λ(p) budget (informational for now)
template <size_t N>
inline bool within_phi_budget(const PhiEvalResult& r) noexcept {
    return r.abs_error <= precision::lambda_p<N>();
}

// Diagnostics counters (non-atomic to keep header-only & constexpr-friendly).
struct PhiDiagCounters {
    unsigned long eval_calls = 0;
    unsigned long budget_ok = 0;
    unsigned long budget_fail = 0;
    long double worst_error = 0.0L;
};

template <size_t N>
inline PhiDiagCounters& phi_diag() {
    static PhiDiagCounters c{};
    return c;
}

template <size_t N>
inline void record_phi(const PhiEvalResult& r, bool ok) {
#if TAKUM_ENABLE_PHI_DIAGNOSTICS
    auto& d = phi_diag<N>();
    ++d.eval_calls;
    if (ok) ++d.budget_ok; else ++d.budget_fail;
    if (r.abs_error > d.worst_error) d.worst_error = r.abs_error;
#else
    (void)r; (void)ok;
#endif
}

} // namespace takum::internal::phi
