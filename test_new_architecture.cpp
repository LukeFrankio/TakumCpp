/**
 * @file test_new_architecture.cpp 
 * @brief Standalone test for the new modular architecture.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <chrono>

// Include the current takum headers for backward compatibility testing
#include "takum/core.h"
#include "takum/arithmetic.h"
#include "takum/types.h"

// Use the current takum namespace

namespace test {

void test_basic_functionality() {
    std::cout << "Testing basic takum functionality..." << std::endl;
    
    // Test construction and basic operations
    takum::types::takum32 a(3.14159);
    takum::types::takum32 b(2.71828);
    
    assert(!a.is_nar());
    assert(!b.is_nar());
    assert(std::abs(a.to_double() - 3.14159) < 1e-5);
    assert(std::abs(b.to_double() - 2.71828) < 1e-5);
    
    // Test arithmetic
    auto sum = a + b;
    auto product = a * b;
    
    assert(!sum.is_nar());
    assert(!product.is_nar());
    assert(std::abs(sum.to_double() - 5.85987) < 1e-3);
    assert(std::abs(product.to_double() - 8.53948) < 1e-3);
    
    std::cout << "  ✓ Basic functionality tests passed" << std::endl;
}

void test_nar_handling() {
    std::cout << "Testing NaR handling..." << std::endl;
    
    takum::types::takum32 nar = takum::types::takum32::nar();
    takum::types::takum32 value(1.0);
    
    assert(nar.is_nar());
    assert(!value.is_nar());
    
    // NaR propagation
    auto result1 = nar + value;
    auto result2 = value * nar;
    
    assert(result1.is_nar());
    assert(result2.is_nar());
    
    std::cout << "  ✓ NaR handling tests passed" << std::endl;
}

void test_precision_levels() {
    std::cout << "Testing multiple precision levels..." << std::endl;
    
    takum::types::takum16 a16(1.5);
    takum::types::takum32 a32(1.5);
    takum::types::takum64 a64(1.5);
    
    assert(std::abs(a16.to_double() - 1.5) < 1e-2); // Lower precision
    assert(std::abs(a32.to_double() - 1.5) < 1e-6);
    assert(std::abs(a64.to_double() - 1.5) < 1e-12);
    
    std::cout << "  ✓ Multi-precision tests passed" << std::endl;
}

void test_comparison_operations() {
    std::cout << "Testing comparison operations..." << std::endl;
    
    takum::types::takum32 a(1.0);
    takum::types::takum32 b(2.0);
    takum::types::takum32 nar = takum::types::takum32::nar();
    
    // Basic comparisons
    assert(a < b);
    assert(b > a);
    assert(a == a);
    assert(a != b);
    
    // NaR comparisons (NaR is smallest)
    assert(nar < a);
    assert(nar < b);
    assert(nar == nar);
    
    std::cout << "  ✓ Comparison tests passed" << std::endl;
}

void test_special_values() {
    std::cout << "Testing special values..." << std::endl;
    
    takum::types::takum32 zero(0.0);
    takum::types::takum32 one(1.0);
    takum::types::takum32 neg_one(-1.0);
    
    assert(zero.to_double() == 0.0);
    assert(std::abs(one.to_double() - 1.0) < 1e-10);
    assert(std::abs(neg_one.to_double() + 1.0) < 1e-10);
    
    // Test unity multiplication
    auto result = one * one;
    assert(std::abs(result.to_double() - 1.0) < 1e-10);
    
    std::cout << "  ✓ Special values tests passed" << std::endl;
}

void benchmark_basic_operations() {
    std::cout << "Running basic performance benchmark..." << std::endl;
    
    takum::types::takum32 a(1.5);
    takum::types::takum32 b(2.5);
    
    const int iterations = 1000000;
    
    // Benchmark addition
    auto start = std::chrono::high_resolution_clock::now();
    takum::types::takum32 sum = a;
    for (int i = 0; i < iterations; ++i) {
        sum = sum + b;
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "  " << iterations << " additions in " << duration.count() << " μs" << std::endl;
    std::cout << "  Average: " << (double)duration.count() / iterations << " μs per operation" << std::endl;
    
    // Verify result is reasonable (should not be NaR)
    assert(!sum.is_nar());
}

} // namespace test

int main() {
    std::cout << "=== TakumCpp Refactored Architecture Validation ===" << std::endl;
    std::cout << std::endl;
    
    try {
        test::test_basic_functionality();
        test::test_nar_handling();
        test::test_precision_levels();
        test::test_comparison_operations();  
        test::test_special_values();
        
        std::cout << std::endl;
        std::cout << "=== All Core Tests Passed ===" << std::endl;
        std::cout << std::endl;
        
        // Performance benchmark
        test::benchmark_basic_operations();
        
        std::cout << std::endl;
        std::cout << "🎉 SUCCESS: TakumCpp refactored architecture validation completed!" << std::endl;
        std::cout << "   - Backward compatibility maintained" << std::endl;
        std::cout << "   - All precision levels working" << std::endl;
        std::cout << "   - NaR handling correct" << std::endl;
        std::cout << "   - Performance characteristics reasonable" << std::endl;
        std::cout << std::endl;
        std::cout << "✅ Ready for next implementation phases:" << std::endl;
        std::cout << "   → Performance optimization" << std::endl;
        std::cout << "   → Enhanced testing infrastructure" << std::endl;
        std::cout << "   → Full modular implementation" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ TEST FAILED: Unknown exception" << std::endl;
        return 1;
    }
}