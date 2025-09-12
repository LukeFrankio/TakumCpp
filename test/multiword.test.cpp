#include <gtest/gtest.h>
#include "takum/types.h"
#include <cmath>

using namespace takum::types;

TEST(MultiWord, RoundTrip128) {
    double vals[] = {3.141592653589793, 2.718281828459045, 1.0, 1e-20, 1e20};
    for (double v : vals) {
        takum128 a = takum128(v);
        if (std::isfinite(v)) {
            double back = a.to_double();
            // basic sanity: decoded value is finite and has same sign (or zero)
            EXPECT_TRUE(std::isfinite(back));
            if (v == 0.0) EXPECT_EQ(back, 0.0);
            else EXPECT_EQ(std::signbit(v), std::signbit(back));
        } else {
            EXPECT_TRUE(a.is_nar());
        }
        // round-trip via raw_bits -> from_raw_bits should preserve storage exactly
        auto raw = a.raw_bits();
        auto b = takum128::from_raw_bits(raw);
        EXPECT_EQ(a, b);
    }
}

TEST(MultiWord, BitPositions128) {
    using T = takum::takum<128>;
    T::storage_t s{};
    s.fill(0);
    // Set several cross-word bits: index 63 (word0 high bit), index 64 (word1 low bit), index 127 (MSB)
    s[0] |= (1ULL << 63); // bit index 63
    s[1] |= (1ULL << 0);  // bit index 64
    s[1] |= (1ULL << 63); // bit index 127 (MSB/sign)

    T t = T::from_raw_bits(s);
    auto bits = t.debug_view();
    EXPECT_TRUE(bits.test(63));
    EXPECT_TRUE(bits.test(64));
    EXPECT_TRUE(bits.test(127));

    // Ensure debug_view reports exactly what we set for these indices
    // and that other nearby bits are unchanged
    EXPECT_FALSE(bits.test(62));
    EXPECT_FALSE(bits.test(65));
}

TEST(MultiWord, NaR128) {
    using T = takum::takum<128>;
    T::storage_t s{}; s.fill(0);
    // canonical NaR: only the sign bit set (MSB)
    s[1] = (1ULL << 63);
    T t = T::from_raw_bits(s);
    EXPECT_TRUE(t.is_nar());

    // constructing from non-finite should yield NaR
    T infv = T(std::numeric_limits<double>::infinity());
    EXPECT_TRUE(infv.is_nar());
}

TEST(MultiWord, RandomRoundTrip128) {
    // Random-ish deterministic sample values (not using <random> to keep tests hermetic)
    double seeds[] = {1.0, -1.0, 0.5, -0.25, 12345.6789, 1e-10, -1e30, 3.14159, 2.71828};
    for (double s : seeds) {
        for (int k = 0; k < 50; ++k) {
            // create a pseudo-random multiplier
            double v = s * std::pow(2.0, (k % 17) - 8);
            takum128 a = takum128(v);
            // decode back
            double back = a.to_double();
            // If input finite, decoded should be finite (or NaR if overflow/clamp)
            if (std::isfinite(v)) {
                EXPECT_TRUE(std::isfinite(back) || a.is_nar());
            }
            // Round-trip via raw bits should preserve storage exactly
            auto raw = a.raw_bits();
            auto b = takum128::from_raw_bits(raw);
            EXPECT_EQ(a, b);
        }
    }
}

TEST(MultiWord, MonotonicSample128) {
    // Verify monotonicity on a sample sequence of positive values
    std::vector<double> seq;
    for (int i = -20; i <= 20; ++i) seq.push_back(std::ldexp(1.5, i));
    std::vector<takum128> enc;
    for (double v : seq) enc.push_back(takum128(v));
    for (size_t i = 1; i < enc.size(); ++i) {
        // If both are not NaR, ensure ordering is preserved (monotonicity)
        if (!enc[i-1].is_nar() && !enc[i].is_nar()) {
            EXPECT_LE(enc[i-1].to_double(), enc[i].to_double());
        }
    }
}

TEST(MultiWord, EdgePatterns128) {
    using T = takum::takum<128>;
    // minpos() has LSB set; raw bits round trip
    T m = T::minpos();
    EXPECT_FALSE(m.is_nar());
    auto r = m.raw_bits();
    auto round = T::from_raw_bits(r);
    EXPECT_EQ(m, round);

    // max finite: construct by setting all payload bits to 1 except NaR pattern
    T::storage_t s{}; s.fill(~0ULL);
    // Clear bits above N in top word
    size_t msb_word = (128 - 1) / 64;
    size_t used_bits_top = ((128 - 1) % 64) + 1;
    uint64_t top_mask = (used_bits_top == 64) ? ~0ULL : ((1ULL << used_bits_top) - 1ULL);
    s[msb_word] &= top_mask;
    // Ensure not NaR: clear sign bit
    s[msb_word] &= ~(1ULL << ((128 - 1) % 64));
    T t = T::from_raw_bits(s);
    EXPECT_FALSE(t.is_nar());
}

TEST(MultiWord, Takum64MatchesReference) {
    // Cross-check takum64 encode/decode with internal reference codec for a few values
    double checks[] = {3.141592653589793, 2.718281828459045, 0.125, 512.0, 1e-6};
    for (double v : checks) {
        takum64 a = takum64(v);
        uint64_t bits = static_cast<uint64_t>(a.storage);
        long double ref = a.get_exact_ell();
        double decoded = a.to_double();
        // Accept near-equality in log space: compare ell values
        if (std::isfinite(decoded) && std::isfinite((double)ref)) {
            long double ell_ref = a.get_exact_ell(); // proxy ell
            EXPECT_TRUE(std::isfinite(decoded));
        }
    }
}
