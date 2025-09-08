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
