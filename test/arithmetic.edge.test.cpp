#include <gtest/gtest.h>
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

TEST(ArithmeticEdge, ZeroAndMinpos) {
    takum128 z = takum128(0.0);
    EXPECT_FALSE(z.is_nar());
    EXPECT_TRUE(z.debug_view().none());

    takum128 mp = takum128::minpos();
    EXPECT_FALSE(mp.is_nar());
    EXPECT_FALSE(mp.debug_view().none());
    EXPECT_TRUE(mp.debug_view().test(0));
    auto r = mp.raw_bits();
    auto r2 = takum128::from_raw_bits(r);
    EXPECT_EQ(mp, r2);
}

TEST(ArithmeticEdge, DivisionByZeroYieldsNaR) {
    takum128 a = takum128(1.0);
    takum128 zero = takum128(0.0);
    auto res = a / zero;
    EXPECT_TRUE(res.is_nar());
}

TEST(ArithmeticEdge, LargeOverflowBecomesNaR) {
    // Extremely large magnitude should map to NaR per encode rules (clamp/NaR)
    takum128 t = takum128(1e300);
    EXPECT_TRUE(t.is_nar());
}

TEST(ArithmeticEdge, NaRRoundTrip) {
    takum128 n = takum128::nar();
    EXPECT_TRUE(n.is_nar());
    auto raw = n.raw_bits();
    auto n2 = takum128::from_raw_bits(raw);
    EXPECT_TRUE(n2.is_nar());
    EXPECT_EQ(n, n2);
}
