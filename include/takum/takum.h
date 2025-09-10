/**
 * @file takum.h
 * @brief Main takum type definition using the refactored modular architecture.
 *
 * This header provides the user-facing takum<N> type that leverages the new
 * modular architecture with separated concerns:
 * - Core encoding/decoding operations
 * - Configurable arithmetic engine with Φ strategies
 * - Runtime configuration system
 * - Clean API with immutable value semantics
 */
#pragma once

#include "takum/core/encoding.h"
#include "takum/arithmetic/arithmetic_engine.h"
#include "takum/config/runtime_config.h"
#include <optional>
#include <string>
#include <iostream>

namespace takum {

/**
 * @brief Error types for safe takum operations.
 */
enum class takum_error_kind {
    domain_error,      ///< Input outside valid domain
    overflow,          ///< Result too large to represent
    underflow,         ///< Result too small to represent  
    invalid_operation, ///< Operation not defined (e.g., 0/0)
    inexact,          ///< Result cannot be represented exactly
    internal_error    ///< Internal computation error
};

/**
 * @brief Error information for failed takum operations.
 */
struct takum_error {
    takum_error_kind kind;
    std::string message;
    
    takum_error(takum_error_kind k, std::string msg = "")
        : kind(k), message(std::move(msg)) {}
};

// Forward declaration for expected-style return types
template<typename T> using expected = std::optional<T>;

/**
 * @brief Main takum numeric type with modular architecture.
 *
 * This class provides a clean user interface while leveraging the modular
 * backend architecture. Key design principles:
 * - Immutable value semantics
 * - Configurable arithmetic strategies
 * - Thread-safe operations
 * - Comprehensive error handling
 */
template<size_t N>
class takum {
public:
    using storage_type = typename core::storage_traits<N>::type;
    using encoder_type = core::encoder<N>;
    using arithmetic_engine_type = arithmetic::arithmetic_engine<N>;
    
    static constexpr size_t bit_width = N;
    
    // ========================================================================
    // Constructors and Basic Operations
    // ========================================================================
    
    /**
     * @brief Default constructor - creates zero value.
     */
    constexpr takum() noexcept : bits_(storage_type{0}) {}
    
    /**
     * @brief Construct from real value.
     */
    explicit takum(double value) noexcept 
        : bits_(encoder_type::encode(value)) {}
    
    /**
     * @brief Construct from storage bits (internal use).
     */
    explicit constexpr takum(storage_type bits) noexcept : bits_(bits) {}
    
    /**
     * @brief Copy constructor.
     */
    constexpr takum(const takum&) noexcept = default;
    
    /**
     * @brief Copy assignment.
     */
    constexpr takum& operator=(const takum&) noexcept = default;
    
    /**
     * @brief Create NaR (Not-a-Real) value.
     */
    static constexpr takum nar() noexcept {
        return takum{encoder_type::nar_pattern()};
    }
    
    /**
     * @brief Create zero value.
     */
    static constexpr takum zero() noexcept {
        return takum{storage_type{0}};
    }
    
    /**
     * @brief Create unity (1.0) value.
     */
    static takum one() noexcept {
        return takum{1.0};
    }
    
    // ========================================================================
    // Value Access and Conversion
    // ========================================================================
    
    /**
     * @brief Convert to double precision.
     */
    double to_double() const noexcept {
        return encoder_type::decode(bits_);
    }
    
    /**
     * @brief Get exact logarithmic (ℓ) representation.
     */
    long double get_exact_ell() const noexcept {
        return encoder_type::extract_ell(bits_);
    }
    
    /**
     * @brief Check if value is NaR (Not-a-Real).
     */
    constexpr bool is_nar() const noexcept {
        return encoder_type::is_nar(bits_);
    }
    
    /**
     * @brief Check if value is zero.
     */
    constexpr bool is_zero() const noexcept {
        return bits_ == storage_type{0};
    }
    
    /**
     * @brief Check if value is finite (not NaR).
     */
    constexpr bool is_finite() const noexcept {
        return !is_nar();
    }
    
    /**
     * @brief Get internal bit representation.
     */
    constexpr storage_type bits() const noexcept {
        return bits_;
    }
    
    // ========================================================================
    // Arithmetic Operations (using configurable engine)
    // ========================================================================
    
    /**
     * @brief Addition operator.
     */
    takum operator+(const takum& other) const noexcept {
        return takum{get_arithmetic_engine().add(bits_, other.bits_)};
    }
    
    /**
     * @brief Subtraction operator.
     */
    takum operator-(const takum& other) const noexcept {
        return takum{get_arithmetic_engine().subtract(bits_, other.bits_)};
    }
    
    /**
     * @brief Multiplication operator (exact in ℓ-space).
     */
    takum operator*(const takum& other) const noexcept {
        if (is_nar() || other.is_nar()) return nar();
        if (is_zero() || other.is_zero()) return zero();
        
        // Multiplication is exact in ℓ-space: ℓ_result = ℓ_a + ℓ_b
        long double ell_a = get_exact_ell();
        long double ell_b = other.get_exact_ell();
        long double ell_result = ell_a + ell_b;
        
        // Convert back to real value
        double result = std::exp(ell_result / 2.0);
        
        // Handle sign
        bool sign_a = encoder_type::extract_sign(bits_);
        bool sign_b = encoder_type::extract_sign(other.bits_);
        if (sign_a != sign_b) result = -result;
        
        return takum{result};
    }
    
    /**
     * @brief Division operator (exact in ℓ-space).
     */
    takum operator/(const takum& other) const noexcept {
        if (is_nar() || other.is_nar()) return nar();
        if (other.is_zero()) return nar(); // Division by zero
        if (is_zero()) return zero();
        
        // Division is exact in ℓ-space: ℓ_result = ℓ_a - ℓ_b
        long double ell_a = get_exact_ell();
        long double ell_b = other.get_exact_ell();
        long double ell_result = ell_a - ell_b;
        
        // Convert back to real value
        double result = std::exp(ell_result / 2.0);
        
        // Handle sign
        bool sign_a = encoder_type::extract_sign(bits_);
        bool sign_b = encoder_type::extract_sign(other.bits_);
        if (sign_a != sign_b) result = -result;
        
        return takum{result};
    }
    
    /**
     * @brief Unary negation.
     */
    takum operator-() const noexcept {
        if (is_nar() || is_zero()) return *this;
        
        // Flip sign bit
        if constexpr (core::storage_traits<N>::is_single_word) {
            return takum{bits_ ^ (storage_type{1} << (N - 1))};
        } else {
            storage_type result = bits_;
            result[core::storage_traits<N>::word_count - 1] ^= 
                uint64_t{1} << ((N - 1) % 64);
            return takum{result};
        }
    }
    
    /**
     * @brief Absolute value.
     */
    takum abs() const noexcept {
        if (is_nar() || is_zero()) return *this;
        
        // Clear sign bit
        if constexpr (core::storage_traits<N>::is_single_word) {
            return takum{bits_ & ~(storage_type{1} << (N - 1))};
        } else {
            storage_type result = bits_;
            result[core::storage_traits<N>::word_count - 1] &= 
                ~(uint64_t{1} << ((N - 1) % 64));
            return takum{result};
        }
    }
    
    // ========================================================================
    // Comparison Operations
    // ========================================================================
    
    /**
     * @brief Equality comparison.
     */
    constexpr bool operator==(const takum& other) const noexcept {
        return bits_ == other.bits_;
    }
    
    /**
     * @brief Inequality comparison.
     */
    constexpr bool operator!=(const takum& other) const noexcept {
        return !(*this == other);
    }
    
    /**
     * @brief Less-than comparison (follows total ordering with NaR smallest).
     */
    constexpr bool operator<(const takum& other) const noexcept {
        return compare_bits(bits_, other.bits_) < 0;
    }
    
    /**
     * @brief Less-than-or-equal comparison.
     */
    constexpr bool operator<=(const takum& other) const noexcept {
        return compare_bits(bits_, other.bits_) <= 0;
    }
    
    /**
     * @brief Greater-than comparison.
     */
    constexpr bool operator>(const takum& other) const noexcept {
        return compare_bits(bits_, other.bits_) > 0;
    }
    
    /**
     * @brief Greater-than-or-equal comparison.
     */
    constexpr bool operator>=(const takum& other) const noexcept {
        return compare_bits(bits_, other.bits_) >= 0;
    }
    
    // ========================================================================
    // Safe Operations (with error handling)
    // ========================================================================
    
    /**
     * @brief Safe addition with error reporting.
     */
    expected<takum> safe_add(const takum& other) const noexcept {
        takum result = *this + other;
        if (result.is_nar() && !is_nar() && !other.is_nar()) {
            return std::nullopt; // Error occurred
        }
        return result;
    }
    
    /**
     * @brief Safe subtraction with error reporting.
     */
    expected<takum> safe_subtract(const takum& other) const noexcept {
        takum result = *this - other;
        if (result.is_nar() && !is_nar() && !other.is_nar()) {
            return std::nullopt; // Error occurred
        }
        return result;
    }
    
    /**
     * @brief Safe multiplication with error reporting.
     */
    expected<takum> safe_multiply(const takum& other) const noexcept {
        takum result = *this * other;
        if (result.is_nar() && !is_nar() && !other.is_nar()) {
            return std::nullopt; // Error occurred
        }
        return result;
    }
    
    /**
     * @brief Safe division with error reporting.
     */
    expected<takum> safe_divide(const takum& other) const noexcept {
        if (other.is_zero()) {
            return std::nullopt; // Division by zero
        }
        takum result = *this / other;
        if (result.is_nar() && !is_nar() && !other.is_nar()) {
            return std::nullopt; // Error occurred
        }
        return result;
    }
    
    // ========================================================================
    // Configuration and Strategy Access
    // ========================================================================
    
    /**
     * @brief Get information about current arithmetic strategy.
     */
    static std::string get_arithmetic_strategy_info() {
        auto& engine = get_arithmetic_engine();
        auto& strategy = engine.get_strategy();
        return std::string(strategy.strategy_name()) + 
               " (accuracy: " + std::to_string(strategy.accuracy_bound()) + ")";
    }
    
    /**
     * @brief Configure arithmetic strategy for this precision.
     */
    static void configure_arithmetic_strategy(const std::string& strategy_name) {
        config::options::set_phi_strategy(strategy_name);
        // Force recreation of arithmetic engine with new strategy
        arithmetic_engine_cache_.reset();
    }
    
private:
    storage_type bits_;
    
    // Thread-local arithmetic engine cache
    static thread_local std::unique_ptr<arithmetic_engine_type> arithmetic_engine_cache_;
    
    static arithmetic_engine_type& get_arithmetic_engine() {
        if (!arithmetic_engine_cache_) {
            arithmetic_engine_cache_ = std::make_unique<arithmetic_engine_type>();
        }
        return *arithmetic_engine_cache_;
    }
    
    // Helper for bit comparison with proper sign handling
    static constexpr int compare_bits(storage_type a, storage_type b) noexcept {
        // NaR is smallest value
        bool a_is_nar = encoder_type::is_nar(a);
        bool b_is_nar = encoder_type::is_nar(b);
        
        if (a_is_nar && b_is_nar) return 0;
        if (a_is_nar) return -1;
        if (b_is_nar) return 1;
        
        // Use two's complement ordering for remaining values
        if constexpr (core::storage_traits<N>::is_single_word) {
            // Convert to signed for proper comparison
            using signed_type = std::make_signed_t<storage_type>;
            signed_type signed_a = static_cast<signed_type>(a);
            signed_type signed_b = static_cast<signed_type>(b);
            
            if (signed_a < signed_b) return -1;
            if (signed_a > signed_b) return 1;
            return 0;
        } else {
            // Multi-word comparison (more complex, simplified here)
            // TODO: Implement proper multi-word signed comparison
            if (a < b) return -1;
            if (a > b) return 1;
            return 0;
        }
    }
};

// Thread-local storage definition
template<size_t N>
thread_local std::unique_ptr<typename takum<N>::arithmetic_engine_type> 
    takum<N>::arithmetic_engine_cache_;

// ============================================================================
// Stream I/O Operations
// ============================================================================

template<size_t N>
std::ostream& operator<<(std::ostream& os, const takum<N>& t) {
    if (t.is_nar()) {
        return os << "NaR";
    }
    return os << t.to_double();
}

template<size_t N>
std::istream& operator>>(std::istream& is, takum<N>& t) {
    double value;
    if (is >> value) {
        t = takum<N>{value};
    }
    return is;
}

// ============================================================================
// Common Type Aliases
// ============================================================================

using takum16 = takum<16>;
using takum32 = takum<32>;
using takum64 = takum<64>;
using takum128 = takum<128>;

// ============================================================================
// Convenience Functions
// ============================================================================

/**
 * @brief Create takum from string representation.
 */
template<size_t N>
takum<N> from_string(const std::string& str) {
    if (str == "NaR" || str == "nar") {
        return takum<N>::nar();
    }
    try {
        double value = std::stod(str);
        return takum<N>{value};
    } catch (...) {
        return takum<N>::nar();
    }
}

/**
 * @brief Convert takum to string representation.
 */
template<size_t N>
std::string to_string(const takum<N>& t) {
    if (t.is_nar()) {
        return "NaR";
    }
    return std::to_string(t.to_double());
}

} // namespace takum