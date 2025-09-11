/**
 * @file phi_lut.h
 * @brief Lightweight on-demand generated lookup tables for Φ (Gaussian-log) approximation.
 *
 * This header provides optimized lookup table generation and interpolation for the
 * Gaussian-log function Φ used in takum addition operations. The implementation
 * supports both linear and cubic Catmull-Rom interpolation modes for different
 * accuracy/performance trade-offs.
 *
 * @details
 * The lookup tables operate over the domain [-0.5, 0.5] with uniform sampling
 * including both endpoints. Values are stored in Q16 fixed-point format (uint32_t)
 * for deterministic cross-platform representation.
 *
 * **Interpolation Modes:**
 * - **Linear (default)**: Always enabled, conservative error bounds, fastest
 * - **Cubic Catmull-Rom**: Enable with TAKUM_ENABLE_CUBIC_PHI_LUT, higher quality
 *
 * Cubic mode produces smoother derivatives and typically halves maximum error
 * compared to linear interpolation for the same LUT size.
 *
 * @deprecated The term "Phase-4 interim implementation" is deprecated. This
 * provides the current takum Φ function implementation using on-demand lookup
 * tables and can be replaced by offline generated headers for improved
 * reproducibility in production builds.
 *
 * @note This is an interim implementation that can be replaced by offline
 *       generated headers for improved reproducibility in production builds.
 *
 * @see takum::config::cubic_phi_lut() for runtime configuration queries
 * @see TAKUM_ENABLE_CUBIC_PHI_LUT for compile-time feature control
 */

#pragma once

#include <array>
#include <cstdint>
#include <cmath>
#include "takum/internal/phi_types.h"
#include "takum/config.h"

/**
 * @namespace takum::internal::phi
 * @brief Internal implementation namespace for Φ (Gaussian-log) function evaluation.
 *
 * This namespace contains the core implementation details for computing the
 * Gaussian-log function Φ used in high-precision takum addition operations.
 * The implementation provides multiple strategies including lookup tables,
 * polynomial approximation, and hybrid approaches.
 */
namespace takum::internal::phi {

/**
 * @namespace takum::internal::phi::detail
 * @brief Implementation details for Φ lookup table generation and management.
 *
 * Contains low-level utilities for generating, storing, and accessing lookup
 * tables used in Φ function approximation. These components are not intended
 * for direct use outside the phi evaluation system.
 */
namespace detail {
    /// @brief Minimum value of the Φ function domain [-0.5, 0.5]
    constexpr long double domain_min = -0.5L;
    /// @brief Maximum value of the Φ function domain [-0.5, 0.5]  
    constexpr long double domain_max =  0.5L;
    /// @brief Total span of the Φ function domain (always 1.0)
    constexpr long double span = domain_max - domain_min; // 1.0

    /**
     * @brief Reference implementation of the Φ (Gaussian-log) function.
     *
     * Computes Φ(x) = 0.5 * (1 + erf(x/√2)) using the standard library's
     * error function. This serves as the ground truth for lookup table
     * generation and validation.
     *
     * @param x Input value, typically in domain [-0.5, 0.5]
     * @return Φ(x) value in range [0, 1]
     *
     * @note Uses long double precision for maximum accuracy in LUT generation
     */
    inline long double phi_ref(long double x) {
        return 0.5L * (1.0L + std::erfl(x / std::sqrt(2.0L)));
    }

    /**
     * @brief Compile-time lookup table holder for Φ function values.
     *
     * Generates and stores a lookup table with S+1 uniformly spaced samples
     * of the Φ function over the domain [-0.5, 0.5]. Values are stored in
     * Q16 fixed-point format for deterministic cross-platform behavior.
     *
     * @tparam S Number of LUT intervals (total entries = S+1)
     *
     * @details
     * The lookup table is generated at compile time using a constexpr constructor.
     * Values are clamped to [0, 1] for safety and converted to Q16 fixed-point
     * using round-to-nearest for optimal precision within the 16-bit range.
     */
    template <size_t S>
    struct LutHolder {
        /// @brief Storage for Q16 fixed-point Φ function values
        std::array<uint32_t, S+1> data{}; // Q16 values
        
        /**
         * @brief Compile-time constructor that generates the lookup table.
         *
         * Samples the Φ function at S+1 uniformly spaced points over [-0.5, 0.5]
         * and stores the results as Q16 fixed-point values for deterministic
         * cross-platform behavior.
         */
        constexpr LutHolder() : data{} {
            for (size_t i = 0; i <= S; ++i) {
                long double t = domain_min + (span * static_cast<long double>(i) / static_cast<long double>(S));
                long double v = phi_ref(t);
                if (v < 0) v = 0; // clamp safety: floor
                if (v > 1) v = 1; // clamp safety: ceiling
                uint32_t q = static_cast<uint32_t>(std::llround(v * (1ull << 16)));
                data[i] = q;
            }
        }
    };

    /**
     * @brief Retrieves a static lookup table for the specified size.
     *
     * Returns a reference to a statically allocated lookup table with S+1 entries.
     * The table is generated once per template instantiation and reused across
     * all calls for the same size parameter.
     *
     * @tparam S Number of LUT intervals (total entries = S+1)
     * @return Const reference to the lookup table array
     *
     * @note Thread-safe due to static initialization guarantees in C++11+
     */
    template <size_t S>
    inline const std::array<uint32_t, S+1>& get_lut() {
        static const LutHolder<S> holder{}; // constexpr ctor
        return holder.data;
    }

    /**
     * @brief Converts Q16 fixed-point value to long double.
     *
     * @param q Q16 fixed-point value (16 fractional bits)
     * @return Equivalent long double value in range [0, 1]
     */
    inline long double q16_to_ld(uint32_t q) {
        return static_cast<long double>(q) / static_cast<long double>(1ull << 16);
    }

    /**
     * @brief Linear interpolation-based Φ function evaluation using lookup table.
     *
     * Evaluates Φ(t) using linear interpolation between precomputed lookup table
     * values. Provides good accuracy with minimal computational cost.
     *
     * @tparam S Number of LUT intervals (total LUT size = S+1)
     * @param t Input value for Φ evaluation, automatically clamped to [-0.5, 0.5]
     * @return PhiEvalResult containing interpolated value, error bound, and metadata
     *
     * @details
     * The function maps input t to lookup table coordinates, performs linear
     * interpolation between adjacent table entries, and computes a conservative
     * error bound based on local slope magnitude.
     *
     * **Error Bound Calculation:**
     * - Conservative estimate: half of local slope magnitude plus small slack
     * - Accounts for linear interpolation truncation error
     * - Guaranteed to be an upper bound for smooth functions
     *
     * @note Input values outside [-0.5, 0.5] are automatically clamped to domain bounds
     */
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

    /**
     * @brief Cubic Catmull-Rom interpolation-based Φ function evaluation.
     *
     * Evaluates Φ(t) using cubic Catmull-Rom spline interpolation for higher
     * accuracy than linear interpolation. Falls back to linear interpolation
     * if TAKUM_ENABLE_CUBIC_PHI_LUT is not defined.
     *
     * @tparam S Number of LUT intervals (total LUT size = S+1)  
     * @param t Input value for Φ evaluation, automatically clamped to [-0.5, 0.5]
     * @return PhiEvalResult containing interpolated value, error bound, and metadata
     *
     * @details
     * Uses uniform Catmull-Rom spline interpolation that typically provides
     * significantly better accuracy than linear interpolation for smooth functions.
     * The implementation uses 4 adjacent points for interpolation with proper
     * boundary handling.
     *
     * **Catmull-Rom Spline Formula:**
     * - Uses centripetal parameterization equivalent for uniform spacing
     * - Requires 4 control points: y₀, y₁, y₂, y₃
     * - Interpolates between y₁ and y₂ using cubic polynomial
     *
     * **Error Bound Estimation:**
     * - Based on second finite difference magnitude
     * - Accounts for cubic interpolation truncation error
     * - Includes sanity check against linear bound for robustness
     *
     * @note Compilation will use linear fallback if TAKUM_ENABLE_CUBIC_PHI_LUT is not defined
     * @note Input values outside [-0.5, 0.5] are automatically clamped to domain bounds
     */
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
