/**
 * @file refactored_architecture.test.cpp
 * @brief Tests for the new modular architecture implementation.
 */

#include <gtest/gtest.h>

// Test the new modular headers
#include "takum/core/encoding.h"
#include "takum/arithmetic/arithmetic_engine.h"
#include "takum/config/runtime_config.h"
#include "takum/takum.h"

namespace takum {
namespace test {

class RefactoredArchitectureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset configuration to defaults for each test
        config::configuration_manager::instance().reset_all_to_defaults();
    }
};

// Test core encoding functionality
TEST_F(RefactoredArchitectureTest, CoreEncodingBasics) {
    using encoder = core::encoder<32>;
    
    // Test NaR pattern
    auto nar_bits = encoder::nar_pattern();
    EXPECT_TRUE(encoder::is_nar(nar_bits));
    
    // Test zero encoding
    auto zero_bits = encoder::encode(0.0);
    EXPECT_EQ(zero_bits, uint32_t{0});
    EXPECT_FALSE(encoder::is_nar(zero_bits));
    
    // Test basic value encoding/decoding
    double test_value = 3.14159;
    auto encoded = encoder::encode(test_value);
    auto decoded = encoder::decode(encoded);
    
    EXPECT_FALSE(encoder::is_nar(encoded));
    EXPECT_NEAR(decoded, test_value, 1e-6); // Allow for encoding precision
}

// Test new takum type
TEST_F(RefactoredArchitectureTest, NewTakumType) {
    takum32 a(2.0);
    takum32 b(3.0);
    
    // Test basic construction and conversion
    EXPECT_FALSE(a.is_nar());
    EXPECT_FALSE(a.is_zero());
    EXPECT_TRUE(a.is_finite());
    EXPECT_NEAR(a.to_double(), 2.0, 1e-6);
    
    // Test arithmetic operations
    takum32 sum = a + b;
    EXPECT_NEAR(sum.to_double(), 5.0, 1e-5);
    
    takum32 product = a * b;
    EXPECT_NEAR(product.to_double(), 6.0, 1e-5);
    
    // Test NaR handling
    takum32 nar = takum32::nar();
    EXPECT_TRUE(nar.is_nar());
    
    takum32 nar_sum = a + nar;
    EXPECT_TRUE(nar_sum.is_nar());
}

// Test configuration system
TEST_F(RefactoredArchitectureTest, ConfigurationSystem) {
    auto& config_mgr = config::configuration_manager::instance();
    
    // Test setting and getting boolean option
    config_mgr.set<bool>("enable_cubic_interpolation", true);
    EXPECT_TRUE(config_mgr.get<bool>("enable_cubic_interpolation"));
    
    // Test setting and getting size_t option
    config_mgr.set<size_t>("coarse_lut_size", 512);
    EXPECT_EQ(config_mgr.get<size_t>("coarse_lut_size"), 512);
    
    // Test convenience functions
    config::options::set_enable_fast_add(true);
    EXPECT_TRUE(config::options::enable_fast_add());
}

// Test safe arithmetic operations
TEST_F(RefactoredArchitectureTest, SafeArithmetic) {
    takum32 a(1e20);
    takum32 b(1e20);
    
    // Test safe addition
    auto result = a.safe_add(b);
    if (result.has_value()) {
        EXPECT_GT(result.value().to_double(), 0.0);
    }
    
    // Test division by zero
    takum32 zero = takum32::zero();
    auto div_result = a.safe_divide(zero);
    EXPECT_FALSE(div_result.has_value()); // Should fail
}

// Test comparison operations
TEST_F(RefactoredArchitectureTest, ComparisonOperations) {
    takum32 a(1.0);
    takum32 b(2.0);
    takum32 nar = takum32::nar();
    
    // Basic comparisons
    EXPECT_LT(a, b);
    EXPECT_LE(a, b);
    EXPECT_GT(b, a);
    EXPECT_GE(b, a);
    EXPECT_EQ(a, a);
    EXPECT_NE(a, b);
    
    // NaR comparisons (NaR is smallest)
    EXPECT_LT(nar, a);
    EXPECT_LT(nar, b);
    EXPECT_EQ(nar, nar);
}

// Test string conversion
TEST_F(RefactoredArchitectureTest, StringConversion) {
    takum32 value(3.14159);
    
    // Test to_string
    std::string str = to_string(value);
    EXPECT_FALSE(str.empty());
    
    // Test from_string
    takum32 parsed = from_string<32>("2.71828");
    EXPECT_NEAR(parsed.to_double(), 2.71828, 1e-5);
    
    // Test NaR string conversion
    takum32 nar = from_string<32>("NaR");
    EXPECT_TRUE(nar.is_nar());
}

// Test arithmetic strategy information
TEST_F(RefactoredArchitectureTest, ArithmeticStrategy) {
    std::string strategy_info = takum32::get_arithmetic_strategy_info();
    EXPECT_FALSE(strategy_info.empty());
    
    // Test strategy configuration
    EXPECT_NO_THROW(takum32::configure_arithmetic_strategy("polynomial"));
}

// Test different takum precisions
TEST_F(RefactoredArchitectureTest, MultiPrecision) {
    takum16 a16(1.5);
    takum32 a32(1.5);
    takum64 a64(1.5);
    
    EXPECT_NEAR(a16.to_double(), 1.5, 1e-2); // Lower precision
    EXPECT_NEAR(a32.to_double(), 1.5, 1e-6); 
    EXPECT_NEAR(a64.to_double(), 1.5, 1e-12);
    
    // Test that all can perform arithmetic
    auto sum16 = a16 + a16;
    auto sum32 = a32 + a32;
    auto sum64 = a64 + a64;
    
    EXPECT_NEAR(sum16.to_double(), 3.0, 1e-2);
    EXPECT_NEAR(sum32.to_double(), 3.0, 1e-6);
    EXPECT_NEAR(sum64.to_double(), 3.0, 1e-12);
}

} // namespace test
} // namespace takum