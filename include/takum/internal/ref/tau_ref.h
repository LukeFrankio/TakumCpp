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
    static_assert(N >= 12 && N <= 64, "Takum reference encoder supports 12..64 bits");

    const double sqrt_e = std::sqrt(std::exp(1.0));
    const double ln2 = std::log(2.0);
    const double max_exp = 255.0;

    // NaR pattern: S=1, D=0, R=000 (r=7), C=0000000 (7 bits), M all 0 (p=N-12)
    uint64_t nar_pattern = uint64_t(1) << (N - 1); // S=1 at MSB
    if (std::isnan(x)) return nar_pattern;
    if (x == 0.0) return 0u;
    if (!std::isfinite(x)) return nar_pattern; // NaR for inf

    bool s = std::signbit(x);
    double abs_x = std::fabs(x);
    if (abs_x < std::pow(sqrt_e, -max_exp)) return s ? nar_pattern : 0u; // underflow to 0 or -0, but since packed, 0
    if (abs_x > std::pow(sqrt_e, max_exp)) return nar_pattern; // overflow to NaR

    // Decompose x = (-1)^s * (1 + f) * 2^h (IEEE-like frexp for base 2)
    int h_exp;
    double f = std::frexp(abs_x, &h_exp); // f in [0.5, 1), h_exp such that abs_x = f * 2^h_exp
    double significand = 2.0 * f; // now [1, 2)
    double ln_sig = std::log(significand);
    double ell = 2.0 * (h_exp * ln2 + ln_sig); // ℓ = 2 * ln(|x|)

    double signed_ell = s ? -ell : ell;
    int64_t c = std::floor(signed_ell);
    double m = signed_ell - c; // m in [0,1) since floor

    bool D = (c >= 0);
    int64_t abs_c = std::abs(c);
    int r;
    if (D) {
        r = (abs_c == 0) ? 0 : static_cast<int>(std::floor(std::log2(static_cast<double>(abs_c + 1))));
    } else {
        r = (abs_c == 0) ? 0 : static_cast<int>(std::floor(std::log2(static_cast<double>(abs_c))));
    }
    r = std::max(0, std::min(7, r)); // clamp to 0..7

    uint64_t R_val = D ? static_cast<uint64_t>(r) : static_cast<uint64_t>(7 - r);
    uint64_t R = (R_val & 4) >> 2 | (R_val & 2) >> 1 | (R_val & 1);

    // Characteristic bits C (r bits)
    uint64_t C_val;
    if (D) {
        C_val = static_cast<uint64_t>(c - (1LL << r) + 1);
    } else {
        C_val = static_cast<uint64_t>(c + (1LL << (r + 1)) - 1);
    }
    uint64_t C = 0;
    for (int i = 0; i < r; ++i) {
        C |= (C_val & 1u) << i;
        C_val >>= 1;
    }

    // Mantissa bits M (p = N - r - 5 bits)
    int p = N - r - 5;
    if (p < 0) p = 0; // though for N>=12, r<=7, p>=0
    uint64_t M_power = (p == 0) ? 1ULL : (1ULL << p);
    uint64_t M_scaled = static_cast<uint64_t>(std::floor(m * static_cast<double>(M_power)));
    uint64_t M_bits = M_scaled;

    // Pack: S (1), D (1), R (3), C (r), M (p)
    uint64_t bits = 0;
    bits |= static_cast<uint64_t>(s) << (N - 1);
    bits |= static_cast<uint64_t>(D) << (N - 2);
    bits |= (R << (N - 5));
    int char_start = N - 5 - r;
    bits |= (C << char_start);
    int mant_start = char_start - p;
    bits |= (M_bits << mant_start);

    return bits;
}

template <size_t N>
inline double decode_bits_to_double(uint64_t bits) noexcept {
    static_assert(N >= 12 && N <= 64, "Takum reference decoder supports 12..64 bits");

    const double sqrt_e = std::sqrt(std::exp(1.0));

    // NaR: S=1, D=0, R=000, C=0000000 (r=7), M=0...0 (p=N-12)
    bool is_nar = (bits & (1ULL << (N-1))) != 0; // S=1
    uint64_t low_bits = bits & ((1ULL << (N-1)) - 1);
    if (is_nar && low_bits == 0) return std::numeric_limits<double>::quiet_NaN();
    if (bits == 0) return 0.0;

    bool S = (bits & (1ULL << (N-1))) != 0;
    bool D = (bits & (1ULL << (N-2))) != 0;
    uint64_t R = (bits >> (N-5)) & 7u;
    int r = D ? static_cast<int>(R) : 7 - static_cast<int>(R);

    int char_start = N - 5 - r;
    uint64_t C = (bits >> char_start) & ((1ULL << r) - 1);

    int p = N - r - 5;
    int mant_start = char_start - p;
    uint64_t M = (p > 0) ? (bits >> mant_start) & ((1ULL << p) - 1) : 0;

    // Compute c
    int64_t c_sum = 0;
    for (int i = 0; i < r; ++i) {
        c_sum += ((C >> i) & 1) * (1LL << i);
    }
    int64_t c;
    if (D) {
        c = (1LL << r) - 1 + c_sum;
    } else {
        c = - (1LL << (r + 1)) + 1 + c_sum;
    }

    // Compute m
    double m = 0.0;
    if (p > 0) {
        double m_power = static_cast<double>(1ULL << p);
        m = static_cast<double>(M) / m_power;
    }

    double ell = static_cast<double>(c) + m;
    if (S) ell = -ell;

    double y = std::pow(sqrt_e, ell);
    return S ? -y : y;
}

template <size_t N>
inline long double high_precision_decode(uint64_t bits) noexcept {
    static_assert(N >= 12 && N <= 64, "Takum high-precision decoder supports 12..64 bits");

    const long double sqrt_e_ld = std::sqrtl(std::expl(1.0L));

    // NaR check
    bool is_nar = (bits & (1ULL << (N-1))) != 0; // S=1
    uint64_t low_bits = bits & ((1ULL << (N-1)) - 1);
    if (is_nar && low_bits == 0) return std::numeric_limits<long double>::quiet_NaN();
    if (bits == 0) return 0.0L;

    bool S = (bits & (1ULL << (N-1))) != 0;
    bool D = (bits & (1ULL << (N-2))) != 0;
    uint64_t R = (bits >> (N-5)) & 7u;
    int r = D ? static_cast<int>(R) : 7 - static_cast<int>(R);

    int char_start = N - 5 - r;
    uint64_t C = (bits >> char_start) & ((1ULL << r) - 1);

    int p = N - r - 5;
    int mant_start = char_start - p;
    uint64_t M = (p > 0) ? (bits >> mant_start) & ((1ULL << p) - 1) : 0;

    // Compute c
    int64_t c_sum = 0;
    for (int i = 0; i < r; ++i) {
        c_sum += ((C >> i) & 1) * (1LL << i);
    }
    int64_t c;
    if (D) {
        c = (1LL << r) - 1 + c_sum;
    } else {
        c = - (1LL << (r + 1)) + 1 + c_sum;
    }

    // Compute m
    long double m = 0.0L;
    if (p > 0) {
        long double m_power = static_cast<long double>(1ULL << p);
        m = static_cast<long double>(M) / m_power;
    }

    long double ell = static_cast<long double>(c) + m;
    if (S) ell = -ell;

    long double y = std::powl(sqrt_e_ld, ell);
    return S ? -y : y;
}

} // namespace takum::internal::ref
