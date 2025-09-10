#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include "takum/internal/phi_types.h"
#include "takum/config.h"

// Lightweight, on-demand generated LUTs for Φ approximation for small precisions.
// Domain: [-0.5, 0.5]. Uniform samples including both endpoints.
// Stored in Q16 fixed-point (uint32_t) for deterministic representation.
// NOTE: This is a Phase‑4 interim implementation; can be replaced by an
// offline generated header for reproducibility.
//
// Interpolation modes:
//  - Linear (default)   : always enabled, conservative error bound.
//  - Cubic Catmull-Rom  : enable with TAKUM_ENABLE_CUBIC_PHI_LUT (higher quality).
//
// Cubic mode produces a smoother derivative and typically halves the max error
// vs linear for the same LUT size. A conservative error bound is derived from
// local finite differences (falls back to linear bound * 0.6 + slack).

namespace takum::internal::phi {

namespace detail {
    constexpr long double domain_min = -0.5L;
    constexpr long double domain_max =  0.5L;
    constexpr long double span = domain_max - domain_min; // 1.0

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
                if (v < 0) v = 0; if (v > 1) v = 1; // clamp safety
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
