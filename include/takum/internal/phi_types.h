/**
 * @file phi_types.h
 * @brief Type definitions for Φ (Gaussian-log) function evaluation infrastructure.
 *
 * This header defines the core data structures used throughout the Φ function
 * approximation system. These types provide a consistent interface between
 * different evaluation strategies (lookup tables, polynomial approximation,
 * hybrid methods).
 *
 * @details
 * The Φ function evaluation system uses a unified result type that carries
 * both the computed value and associated error bounds. This design enables
 * error budget tracking and adaptive algorithm selection based on accuracy
 * requirements.
 *
 * @see takum::internal::phi::detail for implementation utilities
 * @see takum::internal::phi namespace for evaluation functions
 */

#pragma once

#include <cstdint>

/**
 * @namespace takum::internal::phi
 * @brief Internal implementation namespace for Φ (Gaussian-log) function evaluation.
 *
 * This namespace contains all implementation details for computing the
 * Gaussian-log function Φ(x) = 0.5 * (1 + erf(x/√2)) used in high-precision
 * takum addition operations.
 */
namespace takum::internal::phi {

/**
 * @brief Result bundle for Φ function evaluations with error tracking.
 *
 * This structure encapsulates the result of a Φ function evaluation along
 * with conservative error bounds and metadata useful for error budget
 * analysis and debugging.
 *
 * @details
 * **Design Rationale:**
 * The bundled approach enables:
 * - Consistent error tracking across all evaluation methods
 * - Adaptive algorithm selection based on required accuracy
 * - Debugging and validation of approximation quality
 * - Performance analysis via interval indexing
 *
 * **Error Bound Interpretation:**
 * The abs_error field provides a conservative upper bound such that:
 * |true_phi(t) - value| ≤ abs_error for all inputs in the evaluation domain.
 *
 * **Interval Semantics:**
 * - For LUT methods: index of the interpolation interval
 * - For polynomial methods: index of the domain partition
 * - For hybrid methods: coarse interval index
 */
struct PhiEvalResult {
    /// @brief Approximated Φ(t) function value
    long double value;
    
    /// @brief Conservative absolute error bound: |true_phi(t) - value| ≤ abs_error
    long double abs_error;
    
    /// @brief Interval/cell index for debugging and performance analysis
    /// @details Semantics depend on evaluation strategy:
    /// - LUT: interpolation interval index
    /// - Polynomial: domain partition index  
    /// - Hybrid: coarse domain index
    int interval;
};

} // namespace takum::internal::phi
