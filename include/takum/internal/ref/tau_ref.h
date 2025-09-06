// Reference encoder/decoder for Phase 2
// NOTE: This is a small, readable reference implementation intended for tests and
// verification during Phase 2. It intentionally targets N <= 64 for simplicity.
// It is NOT the final paper-accurate tapered-log encoding; it uses a fixed-point
// scaled natural-log representation as a pragmatic, testable placeholder.

#pragma once

#include <cstdint>
#include <cmath>
#include <limits>

namespace takum::internal::ref {

template <size_t N>
inline uint64_t encode_double_to_bits(double x) noexcept {
    static_assert(N > 1 && N <= 64, "Reference encoder supports 2..64 bits");

    if (std::isnan(x)) {
        // canonical NaR: all-ones in N bits
        const uint64_t maskN = (N >= 64) ? ~uint64_t(0) : (~uint64_t(0) >> (64 - N));
        return maskN;
    }

    if (x == 0.0) return 0u;

    const bool sign = std::signbit(x);
    double y = std::fabs(x);

    // scale factor for fixed-point representation of ln(y)
    constexpr int SCALE = 20; // Q20
    const double q = std::ldexp(1.0, SCALE); // 2^SCALE

    double ln_y = std::log(y);
    // quantize
    int64_t quant = llround(ln_y * q);

    // bias to make non-negative
    const uint64_t payload_bits = N - 1;
    const int64_t bias = int64_t(1) << (payload_bits - 1);
    int64_t biased = quant + bias;

    // clamp
    const int64_t minv = 0;
    const uint64_t maskPayload = (payload_bits >= 64) ? ~uint64_t(0) : (~uint64_t(0) >> (64 - payload_bits));
    const int64_t maxv = int64_t(maskPayload);
    if (biased < minv) biased = minv;
    if (biased > maxv) biased = maxv;
    uint64_t bits = (uint64_t(biased) & maskPayload);
    if (sign) bits |= (uint64_t(1) << payload_bits);
    return bits;
}

template <size_t N>
inline double decode_bits_to_double(uint64_t bits) noexcept {
    static_assert(N > 1 && N <= 64, "Reference decoder supports 2..64 bits");

    // NaR pattern: all ones
    const uint64_t nar_pattern = (N >= 64) ? ~uint64_t(0) : (~uint64_t(0) >> (64 - N));
    if (bits == nar_pattern) return std::numeric_limits<double>::quiet_NaN();
    if (bits == 0) return 0.0;

    const uint64_t payload_bits = N - 1;
    const bool sign = (bits >> payload_bits) & 1u;
    const uint64_t maskPayload = (~uint64_t(0) >> (64 - payload_bits));
    uint64_t payload = bits & maskPayload;

    constexpr int SCALE = 20; // Q20 match encoder
    const double q = std::ldexp(1.0, SCALE);

    const int64_t bias = int64_t(1) << (payload_bits - 1);
    int64_t quant = int64_t(payload) - bias;
    double ln_y = double(quant) / q;
    double y = std::exp(ln_y);
    return sign ? -y : y;
}

} // namespace takum::internal::ref
