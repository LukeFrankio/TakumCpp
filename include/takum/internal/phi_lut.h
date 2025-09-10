#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include "takum/internal/phi_types.h"

// Lightweight, on-demand generated LUTs for Φ approximation for small precisions.
// Domain: [-0.5, 0.5]. Uniform samples including both endpoints.
// Stored in Q16 fixed-point (uint32_t) for deterministic representation.
// NOTE: This is a Phase-4 interim implementation; can be replaced by an
// offline generated header for reproducibility.

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
        long double eb = std::fabsl(v1 - v0) * 0.5L + 1e-7L; // small slack
        return { value, eb, static_cast<int>(i) };
    }
}

// Public small-precision LUT evaluators (S = LUT size)
inline PhiEvalResult phi_lut_1024(long double t) noexcept { return detail::phi_lut_linear<1024>(t); }
inline PhiEvalResult phi_lut_4096(long double t) noexcept { return detail::phi_lut_linear<4096>(t); }

} // namespace takum::internal::phi
