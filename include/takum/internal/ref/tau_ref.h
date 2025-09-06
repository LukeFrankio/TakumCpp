// Reference tapered-log encoder/decoder for Phase 2
//
// This implementation provides a readable, testable tapered-log style codec that
// splits the payload (N-1 bits) into a signed regime field and a fractional
// mantissa field. It uses base = sqrt(e) (so log-domain value ℓ = 2*ln(|x|)).
//
// Notes:
// - It intentionally targets N <= 64 for simplicity and testability.
// - NaR encoding is the canonical all-ones pattern in the low N bits.
// - This is a reference implementation for unit tests and is not optimized.

#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <algorithm>

namespace takum::internal::ref {

template <size_t N>
inline uint64_t encode_double_to_bits(double x) noexcept {
    static_assert(N > 1 && N <= 64, "Reference encoder supports 2..64 bits");

    // NaR: all-ones in the lower N bits
    const uint64_t nar_pattern = (N >= 64) ? ~uint64_t(0) : (~uint64_t(0) >> (64 - N));
    if (std::isnan(x)) return nar_pattern;
    if (x == 0.0) return 0u;
    if (!std::isfinite(x)) return nar_pattern; // saturate/infinite -> NaR by policy

    const bool sign = std::signbit(x);
    double y = std::fabs(x);

    // log-domain in base sqrt(e): ℓ = 2 * ln(y)
    double ell = 2.0 * std::log(y);

    const uint64_t payload_bits = N - 1; // remaining bits after sign

    // split payload into regime (signed) and mantissa (fraction)
    // choose regime bits heuristically to leave at least 1 mantissa bit
    unsigned reg_bits = std::max<unsigned>(1, std::min<unsigned>(payload_bits - 1, std::max<unsigned>(3, payload_bits / 3)));
    unsigned man_bits = unsigned(payload_bits) - reg_bits;

    // regime r = floor(ell)
    int64_t r = (int64_t)std::floor(ell);
    double frac = ell - double(r); // in [0,1)

    // bias regime to unsigned
    int64_t reg_bias = (int64_t(1) << (reg_bits - 1)) - 1; // allows negative regimes
    int64_t reg_max = (int64_t(1) << reg_bits) - 1;
    int64_t reg_biased = r + reg_bias;
    if (reg_biased < 0) reg_biased = 0;
    if (reg_biased > reg_max) reg_biased = reg_max;

    // scale fraction into mantissa bits
    uint64_t man_max = (man_bits >= 64) ? ~uint64_t(0) : ((uint64_t(1) << man_bits) - 1);
    uint64_t man = 0;
    if (man_bits > 0) {
        double scaled = std::nearbyint(frac * double(man_max));
        if (scaled < 0.0) scaled = 0.0;
        if (scaled > double(man_max)) scaled = double(man_max);
        man = uint64_t(scaled);
    }

    uint64_t payload = (uint64_t(reg_biased) << man_bits) | (man & man_max);

    // place sign in the top payload position (bit index = payload_bits)
    uint64_t sign_bit = sign ? (uint64_t(1) << payload_bits) : 0;
    uint64_t out = (payload & ((payload_bits >= 64) ? ~uint64_t(0) : (~uint64_t(0) >> (64 - payload_bits)))) | sign_bit;
    return out;
}

template <size_t N>
inline double decode_bits_to_double(uint64_t bits) noexcept {
    static_assert(N > 1 && N <= 64, "Reference decoder supports 2..64 bits");

    const uint64_t nar_pattern = (N >= 64) ? ~uint64_t(0) : (~uint64_t(0) >> (64 - N));
    if (bits == nar_pattern) return std::numeric_limits<double>::quiet_NaN();
    if (bits == 0) return 0.0;

    const uint64_t payload_bits = N - 1;
    bool sign = ((bits >> payload_bits) & 1u) != 0;
    uint64_t payload_mask = (payload_bits >= 64) ? ~uint64_t(0) : (~uint64_t(0) >> (64 - payload_bits));
    uint64_t payload = bits & payload_mask;

    unsigned reg_bits = std::max<unsigned>(1, std::min<unsigned>(payload_bits - 1, std::max<unsigned>(3, payload_bits / 3)));
    unsigned man_bits = unsigned(payload_bits) - reg_bits;

    uint64_t man_max = (man_bits >= 64) ? ~uint64_t(0) : ((uint64_t(1) << man_bits) - 1);
    uint64_t man = (man_bits > 0) ? (payload & man_max) : 0;
    uint64_t reg_biased = (payload >> man_bits) & ((uint64_t(1) << reg_bits) - 1);

    int64_t reg_bias = (int64_t(1) << (reg_bits - 1)) - 1;
    int64_t r = int64_t(reg_biased) - reg_bias;

    double frac = 0.0;
    if (man_bits > 0) frac = double(man) / double(man_max);

    // reconstruct ell and then value: ell = r + frac, base = sqrt(e) => value = exp(ell/2)
    double ell = double(r) + frac;
    double y = std::exp(ell * 0.5);
    return sign ? -y : y;
}

} // namespace takum::internal::ref
