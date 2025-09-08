#include <gtest/gtest.h>
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

TEST(Arithmetic, BasicOps128) {
    takum128 a = takum128(3.0);
    takum128 b = takum128(2.0);
    EXPECT_EQ((a + b).to_double(), 5.0);
    EXPECT_EQ((a - b).to_double(), 1.0);
    EXPECT_EQ((a * b).to_double(), 6.0);
    EXPECT_EQ((a / b).to_double(), 1.5);
}

TEST(Arithmetic, NaRPropagation) {
    takum128 nanv = takum128(std::numeric_limits<double>::infinity());
    takum128 a = takum128(1.0);
    EXPECT_TRUE((a + nanv).is_nar());
    EXPECT_TRUE((nanv * a).is_nar());
}

TEST(Arithmetic, SafeVariants) {
    takum128 a = takum128(1.5);
    takum128 b = takum128(2.5);
#if __cplusplus >= 202302L
    auto r = takum::safe_add<128>(a, b);
    EXPECT_TRUE(bool(r));
    EXPECT_FALSE(bool(takum::safe_add<128>(a, takum128::nar())));
#else
    auto r = takum::safe_add<128>(a, b);
    EXPECT_TRUE(bool(r));
    EXPECT_FALSE(bool(takum::safe_add<128>(a, takum128::nar())));
#endif
}
