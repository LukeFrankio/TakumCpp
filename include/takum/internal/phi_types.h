/**
 * @file phi_types.h  
 * @brief Type definitions and data structures for Gaussian-log (Φ) evaluation infrastructure.
 *
 * This header defines the core data types used throughout the Φ evaluation system
 * for consistent communication between different approximation strategies (LUT,
 * polynomial, hybrid). The types are designed to carry both the computed values
 * and associated metadata needed for error analysis and optimization.
 *
 * Key principles:
 * - Consistent interface across all Φ evaluation methods
 * - Comprehensive error tracking for Proposition 11 compliance  
 * - Minimal overhead for performance-critical arithmetic operations
 * - Rich metadata for debugging and performance analysis
 */
#pragma once

// Standard library includes for basic types
#include <cstdint>  // Fixed-width integer types for interval indices and counters

/**
 * @namespace takum::internal::phi
 * @brief Internal implementation namespace for Gaussian-log (Φ) evaluation types.
 *
 * Contains all type definitions and data structures used internally by the Φ
 * evaluation system. These types provide a consistent interface for different
 * approximation strategies while carrying essential metadata for error analysis.
 */
namespace takum::internal::phi {

/**
 * @brief Result bundle for internal Φ function evaluations.
 *
 * This structure encapsulates the complete result of a Φ evaluation, including
 * the computed value, error bounds, and metadata needed for further processing.
 * It provides a consistent interface across different evaluation strategies
 * (polynomial approximation, LUT interpolation, hybrid methods).
 *
 * DESIGN RATIONALE:
 * ================
 * The bundled approach allows:
 * - Consistent error propagation across evaluation methods
 * - Rich debugging information for performance analysis
 * - Future extensibility without breaking existing interfaces
 * - Compliance tracking with Proposition 11 accuracy requirements
 *
 * USAGE PATTERN:
 * =============
 * ```cpp
 * PhiEvalResult result = phi_eval<64>(t);
 * long double value = result.value;          // Use computed Φ(t)  
 * long double error = result.abs_error;      // Check error bound
 * int interval_id = result.interval;         // Debug/optimization info
 * ```
 *
 * @note All fields should be treated as read-only after evaluation
 * @note Error bounds are conservative estimates suitable for worst-case analysis
 */
struct PhiEvalResult {
    /**
     * @brief The approximated value of Φ(t).
     * 
     * Contains the computed result of the Φ function evaluation using the
     * selected approximation method (polynomial, LUT, or hybrid). This value
     * is guaranteed to be within the error bounds specified by abs_error.
     */
    long double value;
    
    /**
     * @brief Conservative bound for |Φ_true(t) - value|.
     * 
     * Provides an upper bound on the absolute error between the true mathematical
     * value of Φ(t) and the computed approximation. This bound is conservatively
     * estimated to ensure compliance with Proposition 11 accuracy requirements
     * for the target takum precision.
     * 
     * @note Always non-negative and represents worst-case error estimate
     * @note Used for error budget tracking in compound arithmetic operations
     */
    long double abs_error;
    
    /**
     * @brief Interval or cell index used during evaluation.
     * 
     * Indicates which interval, LUT cell, or polynomial segment was used for
     * the approximation. The exact semantics depend on the evaluation strategy:
     * - Polynomial: Interval index in piecewise approximation  
     * - LUT: Base index for interpolation
     * - Hybrid: Coarse interval index for polynomial selection
     * 
     * This metadata is valuable for:
     * - Performance analysis and optimization
     * - Debugging approximation quality issues
     * - Load balancing across evaluation methods
     * 
     * @note Interpretation depends on the specific evaluation strategy used
     * @note May be used for diagnostic counters and performance profiling
     */
    int interval;
};

} // namespace takum::internal::phi
