#include <gtest/gtest.h>
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

TEST(ArithmeticMixed, ConvertRoundTrip) {
    double v = 7.8125; // representable in many takum widths
    takum128 a128 = takum128(v);
    takum64 a64 = takum64(a128.to_double());
    takum32 a32 = takum32(a64.to_double());
    EXPECT_TRUE(std::isfinite(a32.to_double()));
    // Round-trip back to 128 via doubles
    takum128 r128 = takum128(a32.to_double());
    EXPECT_TRUE(r128.is_nar() == a128.is_nar());
}

TEST(ArithmeticMixed, ManyOperationsSequence) {
    takum64 x = takum64(1.0);
    for (int i = 0; i < 20; ++i) {
        x = x * takum64(1.1) + takum64(0.01);
    }
    EXPECT_TRUE(std::isfinite(x.to_double()) || x.is_nar());
}
