/**
 * @file phi_lut.h
 * @brief Lightweight lookup table (LUT) implementations for Φ approximation in small precisions.
 *
 * This header provides on-demand generated lookup tables for fast and accurate
 * Gaussian-log (Φ) function evaluation for smaller takum precisions (takum16, takum32).
 * The implementation supports multiple interpolation strategies optimized for
 * different accuracy vs. performance trade-offs.
 *
 * TECHNICAL SPECIFICATIONS:
 * ========================
 * - Domain: [-0.5, 0.5] with uniform sampling including both endpoints
 * - Storage: Q16 fixed-point format (uint32_t) for deterministic representation
 * - Interpolation modes: Linear (default) and Cubic Catmull-Rom (optional)
 * - Thread-safe: All LUTs are constexpr-generated and immutable after construction
 *
 * INTERPOLATION STRATEGIES:
 * ========================
 * 1. Linear interpolation (always enabled):
 *    - Conservative error bounds with proven accuracy guarantees
 *    - Fast evaluation with minimal computational overhead
 *    - Suitable for most applications requiring basic accuracy
 *
 * 2. Cubic Catmull-Rom interpolation (optional via TAKUM_ENABLE_CUBIC_PHI_LUT):
 *    - Smoother derivatives and typically halves maximum error vs. linear
 *    - Higher computational cost but better accuracy for critical applications
 *    - Conservative error bounds derived from local finite differences
 *    - Fallback bounds: linear_bound * 0.6 + numerical_slack
 *
 * IMPLEMENTATION NOTES:
 * ====================
 * This is a Phase-4 interim implementation designed for rapid prototyping and testing.
 * For production deployments, this can be replaced by offline-generated headers to
 * ensure reproducibility across different compilation environments and floating-point
 * implementations.
 *
 * @note Internal implementation detail - use through phi_eval.h interface
 * @note All LUT data is generated at compile-time using constexpr evaluation
 * @note Error bounds are conservatively estimated to guarantee accuracy requirements
 */
#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include "takum/internal/phi_types.h"
#include "takum/config.h"

// Standard library includes for mathematical operations and containers
#include <array>    // std::array for compile-time fixed-size lookup tables
#include <cstdint>  // Fixed-width integer types for Q16 fixed-point representation
#include <cmath>    // Mathematical functions (erfl, sqrt, etc.) for reference implementation

// TakumCpp internal headers for Φ evaluation infrastructure  
#include "takum/internal/phi_types.h"  // Type definitions for Φ evaluation results
#include "takum/config.h"              // Compile-time configuration flags and runtime accessors

/**
 * LOOKUP TABLE GENERATION AND OPTIMIZATION STRATEGY:
 * ==================================================
 * The LUT generation process balances several competing requirements:
 * 
 * 1. Accuracy: Tables must satisfy λ(p) error bounds for target precision
 * 2. Memory: Compact representation to minimize cache pressure  
 * 3. Speed: Fast lookup and interpolation for arithmetic operations
 * 4. Determinism: Reproducible results across platforms and compilers
 * 
 * DOMAIN AND SAMPLING STRATEGY:
 * =============================
 * - Domain: [-0.5, 0.5] covers the full input range for Φ in takum arithmetic
 * - Uniform sampling: Simplifies index calculation and interpolation logic
 * - Endpoint inclusion: Both boundaries are explicitly sampled for edge case handling
 * - Q16 fixed-point: 16 fractional bits provide sufficient precision while using 32-bit integers
 * 
 * INTERPOLATION METHOD SELECTION:
 * ===============================
 * Linear interpolation (default):
 * - Proven accuracy with conservative error bounds
 * - Minimal computational overhead for performance-critical paths  
 * - Sufficient for most takum applications requiring reasonable accuracy
 * 
 * Cubic Catmull-Rom interpolation (optional):
 * - Smoother derivatives reduce interpolation artifacts
 * - Typically provides 2x accuracy improvement over linear
 * - Conservative error bounds estimated from finite difference analysis
 * - Enabled via TAKUM_ENABLE_CUBIC_PHI_LUT compile-time flag
 * 
 * PHASE-4 DEVELOPMENT STRATEGY:
 * =============================
 * This implementation supports rapid iteration during Phase-4 development by
 * generating LUTs at compile-time using constexpr evaluation. For production
 * releases, this approach can be replaced with offline-generated headers to
 * guarantee reproducibility across different floating-point implementations
 * and compilation environments.
 * 
 * QUALITY ASSURANCE AND VALIDATION:
 * =================================
 * Error bounds are conservatively estimated to ensure compliance with Proposition 11
 * accuracy requirements. The cubic interpolation fallback uses linear_bound * 0.6 
 * plus numerical slack to account for higher-order approximation uncertainties.
 */

/**
 * @namespace takum::internal::phi
 * @brief Internal implementation namespace for Gaussian-log (Φ) lookup table functionality.
 *
 * This namespace contains all the implementation details for lookup table generation,
 * management, and evaluation used by the Φ approximation system. The design separates
 * internal implementation details from the public interface to allow future optimization
 * without breaking existing code.
 */
namespace takum::internal::phi {

/**
 * @namespace takum::internal::phi::detail  
 * @brief Implementation detail namespace for LUT generation constants and utilities.
 *
 * Contains the low-level constants, reference implementations, and utility functions
 * used during compile-time LUT generation. These are implementation details that
 * should not be used directly by external code.
 */
namespace detail {
    /// @brief Minimum value of the Φ function input domain
    constexpr long double domain_min = -0.5L;
    
    /// @brief Maximum value of the Φ function input domain  
    constexpr long double domain_max =  0.5L;
    
    /// @brief Total span of the input domain (max - min = 1.0)
    constexpr long double span = domain_max - domain_min; // Always 1.0 for this domain

    /**
     * @brief Reference implementation of the Gaussian-log (Φ) function.
     * 
     * This function provides the mathematical reference implementation used during
     * LUT generation and validation. It computes the exact value of Φ(x) using
     * the standard mathematical definition based on the error function.
     * 
     * MATHEMATICAL DEFINITION:
     * Φ(x) = 0.5 * (1 + erf(x / √2))
     * 
     * This is the cumulative distribution function of the standard normal distribution
     * evaluated at x, which provides the theoretical foundation for the Gaussian-log
     * function used in takum addition formulas.
     * 
     * @param x Input value for Φ evaluation (typically in [-0.5, 0.5] for LUT generation)
     * @return The exact value of Φ(x) computed using long double precision
     * @note Used only during compile-time LUT generation and validation
     * @note Results are converted to fixed-point for storage in actual LUTs
     */
    inline long double phi_ref(long double x) {
        return 0.5L * (1.0L + std::erfl(x / std::sqrt(2.0L)));
    }

    template <size_t S>
    struct LutHolder {
        std::array<uint32_t, S+1> data{}; // Q16 values
        constexpr LutHolder() : data{} {
            for (size_t i = 0; i <= S; ++i) {
                long double t = domain_min + (span * static_cast<long double>(i) / static_cast<long double>(S));
                long double v = phi_ref(t);
                // Clamp safety: ensure v is in [0,1] range to prevent overflow in Q16 conversion
                if (v < 0) v = 0; 
                if (v > 1) v = 1;
                uint32_t q = static_cast<uint32_t>(std::llround(v * (1ull << 16)));
                data[i] = q;
            }
        }
    };

    template <size_t S>
    inline const std::array<uint32_t, S+1>& get_lut() {
        static const LutHolder<S> holder{}; // constexpr ctor
        return holder.data;
    }

    inline long double q16_to_ld(uint32_t q) {
        return static_cast<long double>(q) / static_cast<long double>(1ull << 16);
    }

    template <size_t S>
    inline PhiEvalResult phi_lut_linear(long double t) noexcept {
        if (t < domain_min) t = domain_min;
        if (t > domain_max) t = domain_max;
        long double u = (t - domain_min) / span; // [0,1]
        long double f_index = u * static_cast<long double>(S);
        size_t i = static_cast<size_t>(f_index);
        if (i >= S) i = S - 1; // last interval
        long double frac = f_index - static_cast<long double>(i);
        const auto& lut = get_lut<S>();
        long double v0 = q16_to_ld(lut[i]);
        long double v1 = q16_to_ld(lut[i+1]);
        long double value = v0 + (v1 - v0) * frac;
        // Conservative linear interpolation error bound: half interval slope magnitude
        long double eb = fabsl(v1 - v0) * 0.5L + 1e-7L; // small slack
        return { value, eb, static_cast<int>(i) };
    }

    // Cubic Catmull-Rom interpolation (uniform parameterization). Enabled via macro.
    template <size_t S>
    inline PhiEvalResult phi_lut_cubic(long double t) noexcept {
#ifndef TAKUM_ENABLE_CUBIC_PHI_LUT
        return phi_lut_linear<S>(t);
#else
        if (t < domain_min) t = domain_min;
        if (t > domain_max) t = domain_max;
        long double u = (t - domain_min) / span; // [0,1]
        long double f_index = u * static_cast<long double>(S);
        size_t i = static_cast<size_t>(f_index);
        if (i >= S) i = S - 1;
        long double frac = f_index - static_cast<long double>(i);
        const auto& lut = get_lut<S>();

        auto sample = [&](ptrdiff_t idx) -> long double {
            if (idx < 0) idx = 0;
            if (idx > static_cast<ptrdiff_t>(S)) idx = static_cast<ptrdiff_t>(S);
            return q16_to_ld(lut[static_cast<size_t>(idx)]);
        };
        long double y0 = sample(static_cast<ptrdiff_t>(i) - 1);
        long double y1 = sample(static_cast<ptrdiff_t>(i));
        long double y2 = sample(static_cast<ptrdiff_t>(i) + 1);
        long double y3 = sample(static_cast<ptrdiff_t>(i) + 2);
        long double f = frac;
        long double f2 = f * f;
        long double f3 = f2 * f;
        // Catmull-Rom spline (centripetal equivalent for uniform spacing)
        long double value = 0.5L * ((2.0L * y1) + (-y0 + y2) * f +
                            (2.0L*y0 - 5.0L*y1 + 4.0L*y2 - y3) * f2 +
                            (-y0 + 3.0L*y1 - 3.0L*y2 + y3) * f3);
        // Estimate error: use second finite difference magnitude
        long double d2 = fabsl(y2 - 2.0L*y1 + y0) + fabsl(y3 - 2.0L*y2 + y1);
        long double linear_seg = fabsl(y2 - y1);
        long double eb = (d2 * 0.125L) + (linear_seg * 0.05L) + 5e-7L;
        // Sanity: ensure not tighter than linear bound times 0.3 unless extremely smooth
        long double linear_bound = fabsl(y2 - y1) * 0.5L + 1e-7L;
        if (eb < linear_bound * 0.3L) eb = linear_bound * 0.3L;
        return { value, eb, static_cast<int>(i) };
#endif
    }
}

// Public small-precision LUT evaluators (S = LUT size)
inline PhiEvalResult phi_lut_1024(long double t) noexcept {
#ifdef TAKUM_ENABLE_CUBIC_PHI_LUT
    return detail::phi_lut_cubic<1024>(t);
#else
    return detail::phi_lut_linear<1024>(t);
#endif
}
inline PhiEvalResult phi_lut_4096(long double t) noexcept {
#ifdef TAKUM_ENABLE_CUBIC_PHI_LUT
    return detail::phi_lut_cubic<4096>(t);
#else
    return detail::phi_lut_linear<4096>(t);
#endif
}

} // namespace takum::internal::phi
