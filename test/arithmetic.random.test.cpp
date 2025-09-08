#include <gtest/gtest.h>
#include <random>
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

static std::mt19937_64 rng(123456789ull);

template <class T>
void random_ops_test(int iterations=10000) {
    std::uniform_real_distribution<double> dist(-1e3, 1e3);
    for (int i = 0; i < iterations; ++i) {
        double a = dist(rng);
        double b = dist(rng);
        T ta(a), tb(b);
        T r_add = ta + tb;
        T r_mul = ta * tb;
    // Use decoded (quantized) operand values as the operator implementations do.
    double qa = ta.to_double();
    double qb = tb.to_double();
    double ref_add = qa + qb;
    double ref_mul = qa * qb;
    // Construct reference takum from the quantized-sum/product so bitwise
    // equality matches the operator which encodes the intermediate double.
    T ref_t_add(ref_add);
    T ref_t_mul(ref_mul);
    EXPECT_EQ(r_add, ref_t_add);
    EXPECT_EQ(r_mul, ref_t_mul);
    }
}

TEST(ArithmeticRandom, Takum32) { random_ops_test<takum32>(2000); }
TEST(ArithmeticRandom, Takum64) { random_ops_test<takum64>(2000); }
TEST(ArithmeticRandom, Takum128) { random_ops_test<takum128>(2000); }
