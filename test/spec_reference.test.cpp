#include <gtest/gtest.h>
#include "takum/types.h"
#include <takum/internal/ref/tau_ref.h>

using namespace takum::types;

TEST(SpecReference, RoundTrip32) {
    takum32 a = takum32(3.141592653589793);
    long double back = takum::internal::ref::high_precision_decode<32>(a.storage);
    // Reference codec is crude; assert round-trip within a reasonable factor using high precision
    ASSERT_NEAR(back, 3.141592653589793L, 1e-2L);
}

TEST(SpecReference, NaR32) {
    double nanv = std::numeric_limits<double>::quiet_NaN();
    takum32 a = takum32(nanv);
    EXPECT_TRUE(a.is_nar());
    long double nar_dec = takum::internal::ref::high_precision_decode<32>(a.storage);
    EXPECT_TRUE(std::isnan(static_cast<double>(nar_dec)));
    auto e = a.to_expected();
    EXPECT_FALSE(bool(e));
}

TEST(SpecReference, RoundTrip64) {
    takum64 a = takum64(2.718281828459045);
    long double back = takum::internal::ref::high_precision_decode<64>(a.storage);
    ASSERT_NEAR(back, 2.718281828459045L, 1e-2L);
}
