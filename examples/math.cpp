/**
 * @file math.cpp
 * @brief Mathematical functions demonstration for takum<N> (Phase 4 deliverable).
 *
 * This example demonstrates:
 * - Function composition (e.g., sin ∘ exp) 
 * - Hybrid Φ usage indirectly via operations
 * - Mathematical constants usage
 * - Safe variants with error handling
 * - Deprecated function examples (with warnings)
 */

#include "takum/math.h"
#include "takum/math_constants.h"
#include "takum/core.h"
#include "takum/arithmetic.h"
#include "takum/types.h"

#include <iostream>
#include <iomanip>
#include <vector>

using namespace takum::types;

/**
 * @brief Demonstrate basic mathematical functions
 */
void demonstrate_basic_functions() {
    std::cout << "\n=== Basic Mathematical Functions ===\n";
    
    takum64 x(0.5);
    takum64 pi_quarter = takum64(3.14159265359 / 4.0); // π/4 approximation
    
    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input x = " << x.to_double() << "\n";
    std::cout << "Input angle = π/4 ≈ " << pi_quarter.to_double() << "\n\n";
    
    // Trigonometric functions
    std::cout << "Trigonometric functions:\n";
    std::cout << "  sin(x) = " << takum::sin(x).to_double() << "\n";
    std::cout << "  cos(x) = " << takum::cos(x).to_double() << "\n";
    std::cout << "  tan(π/4) = " << takum::tan(pi_quarter).to_double() << " (should be ≈ 1.0)\n";
    std::cout << "  asin(x) = " << takum::asin(x).to_double() << "\n";
    std::cout << "  atan2(1,1) = " << takum::atan2(takum64(1.0), takum64(1.0)).to_double() << "\n\n";
    
    // Exponential and logarithmic functions
    std::cout << "Exponential and logarithmic functions:\n";
    std::cout << "  exp(x) = " << takum::exp(x).to_double() << "\n";
    std::cout << "  log(e≈2.718) = " << takum::log(takum64(2.71828)).to_double() << "\n";
    std::cout << "  log10(100) = " << takum::log10(takum64(100.0)).to_double() << "\n";
    std::cout << "  pow(2,3) = " << takum::pow(takum64(2.0), takum64(3.0)).to_double() << "\n\n";
    
    // Root functions
    std::cout << "Root functions:\n";
    std::cout << "  sqrt(16) = " << takum::sqrt(takum64(16.0)).to_double() << "\n";
    std::cout << "  cbrt(27) = " << takum::cbrt(takum64(27.0)).to_double() << "\n";
    std::cout << "  hypot(3,4) = " << takum::hypot(takum64(3.0), takum64(4.0)).to_double() << "\n\n";
}

/**
 * @brief Demonstrate function composition
 */
void demonstrate_function_composition() {
    std::cout << "\n=== Function Composition ===\n";
    
    takum64 x(0.5);
    takum64 y(4.0);
    
    // Function composition: sin(exp(x))
    auto exp_x = takum::exp(x);
    auto sin_exp_x = takum::sin(exp_x);
    
    std::cout << "Function composition examples:\n";
    std::cout << "  x = " << x.to_double() << "\n";
    std::cout << "  exp(x) = " << exp_x.to_double() << "\n";
    std::cout << "  sin(exp(x)) = " << sin_exp_x.to_double() << "\n\n";
    
    // Another composition: log(sqrt(y))
    auto sqrt_y = takum::sqrt(y);
    auto log_sqrt_y = takum::log(sqrt_y);
    auto half_log_y = takum64(0.5) * takum::log(y);
    
    std::cout << "  y = " << y.to_double() << "\n";
    std::cout << "  sqrt(y) = " << sqrt_y.to_double() << "\n";
    std::cout << "  log(sqrt(y)) = " << log_sqrt_y.to_double() << "\n";
    std::cout << "  0.5 * log(y) = " << half_log_y.to_double() << " (should be equal)\n\n";
}

/**
 * @brief Demonstrate mathematical constants
 */
void demonstrate_constants() {
    std::cout << "\n=== Mathematical Constants ===\n";
    
    std::cout << std::fixed << std::setprecision(15);
    std::cout << "Mathematical constants for takum64:\n";
    
    auto pi_val = takum::math_constants::pi_v<takum64>();
    auto e_val = takum::math_constants::e_v<takum64>();
    auto sqrt2_val = takum::math_constants::sqrt2_v<takum64>();
    
    std::cout << "  π = " << pi_val.to_double() << "\n";
    std::cout << "  e = " << e_val.to_double() << "\n";
    std::cout << "  √2 = " << sqrt2_val.to_double() << "\n\n";
    
    // Verify some mathematical relationships
    std::cout << "Verifying mathematical relationships:\n";
    std::cout << "  sin(π) = " << takum::sin(pi_val).to_double() << " (should be ≈ 0)\n";
    std::cout << "  cos(π) = " << takum::cos(pi_val).to_double() << " (should be ≈ -1)\n";
    std::cout << "  e^1 = " << takum::exp(takum64(1.0)).to_double() << " (should equal e)\n\n";
}

/**
 * @brief Demonstrate safe variants with error handling
 */
void demonstrate_safe_variants() {
    std::cout << "\n=== Safe Variants with Error Handling ===\n";
    
    takum64 positive(4.0);
    takum64 negative(-1.0);
    takum64 zero(0.0);
    takum64 nar_val = takum64::nar();
    
    std::cout << "Safe function calls with valid inputs:\n";
    
#if TAKUM_HAS_STD_EXPECTED
    auto safe_sqrt_pos = takum::safe_sqrt(positive);
    if (safe_sqrt_pos.has_value()) {
        std::cout << "  safe_sqrt(4.0) = " << safe_sqrt_pos.value().to_double() << "\n";
    }
    
    auto safe_log_pos = takum::safe_log(positive);
    if (safe_log_pos.has_value()) {
        std::cout << "  safe_log(4.0) = " << safe_log_pos.value().to_double() << "\n";
    }
    
    std::cout << "\nSafe function calls with invalid inputs (should report errors):\n";
    
    auto safe_sqrt_neg = takum::safe_sqrt(negative);
    if (!safe_sqrt_neg.has_value()) {
        std::cout << "  safe_sqrt(-1.0) -> Error: " << safe_sqrt_neg.error().message << "\n";
    }
    
    auto safe_log_neg = takum::safe_log(negative);
    if (!safe_log_neg.has_value()) {
        std::cout << "  safe_log(-1.0) -> Error: " << safe_log_neg.error().message << "\n";
    }
    
    auto safe_pow_invalid = takum::safe_pow(zero, negative);
    if (!safe_pow_invalid.has_value()) {
        std::cout << "  safe_pow(0.0, -1.0) -> Error: " << safe_pow_invalid.error().message << "\n";
    }
#else
    std::cout << "Safe variants require C++23 std::expected support (not available)\n";
    std::cout << "Using regular functions instead:\n";
    std::cout << "  sqrt(4.0) = " << takum::sqrt(positive).to_double() << "\n";
    std::cout << "  sqrt(-1.0) = " << (takum::sqrt(negative).is_nar() ? "NaR" : "finite") << "\n";
    std::cout << "  log(-1.0) = " << (takum::log(negative).is_nar() ? "NaR" : "finite") << "\n";
#endif
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate domain and range behavior
 */
void demonstrate_domain_behavior() {
    std::cout << "\n=== Domain and Range Behavior ===\n";
    
    std::cout << "Testing domain boundaries:\n";
    
    // asin domain: [-1, 1]
    auto asin_valid = takum::asin(takum64(0.5));
    auto asin_invalid = takum::asin(takum64(1.5));
    std::cout << "  asin(0.5) = " << asin_valid.to_double() << " (valid)\n";
    std::cout << "  asin(1.5) = " << (asin_invalid.is_nar() ? "NaR" : "finite") << " (invalid)\n";
    
    // log domain: (0, ∞)
    auto log_valid = takum::log(takum64(2.0));
    auto log_invalid = takum::log(takum64(-1.0));
    std::cout << "  log(2.0) = " << log_valid.to_double() << " (valid)\n";
    std::cout << "  log(-1.0) = " << (log_invalid.is_nar() ? "NaR" : "finite") << " (invalid)\n";
    
    // sqrt domain: [0, ∞)
    auto sqrt_valid = takum::sqrt(takum64(4.0));
    auto sqrt_invalid = takum::sqrt(takum64(-1.0));
    std::cout << "  sqrt(4.0) = " << sqrt_valid.to_double() << " (valid)\n";
    std::cout << "  sqrt(-1.0) = " << (sqrt_invalid.is_nar() ? "NaR" : "finite") << " (invalid)\n\n";
}

/**
 * @brief Demonstrate rounding and classification functions
 */
void demonstrate_rounding_classification() {
    std::cout << "\n=== Rounding and Classification ===\n";
    
    takum64 x(3.7);
    takum64 y(-2.3);
    takum64 nar_val = takum64::nar();
    
    std::cout << "Rounding functions:\n";
    std::cout << "  x = " << x.to_double() << "\n";
    std::cout << "  floor(x) = " << takum::floor(x).to_double() << "\n";
    std::cout << "  ceil(x) = " << takum::ceil(x).to_double() << "\n";
    std::cout << "  round(x) = " << takum::round(x).to_double() << "\n";
    std::cout << "  trunc(x) = " << takum::trunc(x).to_double() << "\n\n";
    
    std::cout << "Classification functions:\n";
    std::cout << "  isfinite(" << x.to_double() << ") = " << (takum::isfinite(x) ? "true" : "false") << "\n";
    std::cout << "  isnan(" << x.to_double() << ") = " << (takum::isnan(x) ? "true" : "false") << "\n";
    std::cout << "  isinf(" << x.to_double() << ") = " << (takum::isinf(x) ? "true" : "false") << "\n";
    std::cout << "  signbit(" << x.to_double() << ") = " << (takum::signbit(x) ? "true" : "false") << "\n";
    std::cout << "  signbit(" << y.to_double() << ") = " << (takum::signbit(y) ? "true" : "false") << "\n\n";
    
    std::cout << "  isfinite(NaR) = " << (takum::isfinite(nar_val) ? "true" : "false") << "\n";
    std::cout << "  isnan(NaR) = " << (takum::isnan(nar_val) ? "true" : "false") << "\n\n";
}

/**
 * @brief Demonstrate Φ integration through addition operations
 */
void demonstrate_phi_integration() {
    std::cout << "\n=== Hybrid Φ (Gaussian-log) Integration ===\n";
    std::cout << "Note: Φ evaluation is used internally during addition operations\n";
    std::cout << "in compound mathematical functions.\n\n";
    
    takum64 a(1.2);
    takum64 b(0.8);
    
    // This will use Φ internally during the addition
    auto sum = a + b;
    
    // Complex expression that uses multiple additions (and thus Φ evaluations)
    auto complex_expr = takum::sin(a) + takum::cos(b) + takum::exp(takum64(0.1));
    
    std::cout << "Expression using Φ-enhanced addition:\n";
    std::cout << "  a = " << a.to_double() << "\n";
    std::cout << "  b = " << b.to_double() << "\n";
    std::cout << "  a + b = " << sum.to_double() << " (uses Φ internally)\n";
    std::cout << "  sin(a) + cos(b) + exp(0.1) = " << complex_expr.to_double() << "\n\n";
}

int main() {
    std::cout << "TakumCpp Mathematical Functions Demonstration\n";
    std::cout << "============================================\n";
    
    try {
        demonstrate_basic_functions();
        demonstrate_function_composition();
        demonstrate_constants();
        demonstrate_safe_variants();
        demonstrate_domain_behavior();
        demonstrate_rounding_classification();
        demonstrate_phi_integration();
        
        std::cout << "Mathematical functions demonstration completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}