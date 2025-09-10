/**
 * @file arithmetic_engine.h
 * @brief Modular arithmetic engine with pluggable Φ evaluation strategies.
 *
 * This header provides a clean separation between arithmetic operations and
 * the specific strategies used for Φ evaluation. The design enables:
 * - Runtime selection of optimization strategies
 * - Easy benchmarking and comparison
 * - Future extension to GPU acceleration
 * - Isolated testing of different approaches
 */
#pragma once

#include <memory>
#include <cstddef>
#include "takum/core/encoding.h"

namespace takum {
namespace arithmetic {

/**
 * @brief Abstract base class for Φ evaluation strategies.
 *
 * This interface allows different implementations of the Gaussian-log
 * function Φ to be plugged into the arithmetic engine. Strategies can
 * optimize for different criteria:
 * - Accuracy vs speed tradeoffs
 * - Memory usage considerations
 * - Architecture-specific optimizations
 */
class phi_strategy {
public:
    virtual ~phi_strategy() = default;
    
    /**
     * @brief Evaluate Φ function for addition operation.
     *
     * @param t Input parameter (typically in range [-0.5, 0))
     * @return Φ(t) value computed using this strategy
     */
    virtual long double evaluate_phi(long double t) const noexcept = 0;
    
    /**
     * @brief Get strategy name for debugging/benchmarking.
     */
    virtual const char* strategy_name() const noexcept = 0;
    
    /**
     * @brief Get expected accuracy bound for this strategy.
     */
    virtual double accuracy_bound() const noexcept = 0;
    
    /**
     * @brief Check if strategy is suitable for given precision.
     */
    virtual bool supports_precision(size_t bit_width) const noexcept = 0;
};

/**
 * @brief Polynomial-based Φ evaluation strategy.
 *
 * Uses minimax polynomial approximation with pre-computed coefficients.
 * Optimized for high precision requirements (takum64+).
 */
class polynomial_phi_strategy : public phi_strategy {
public:
    explicit polynomial_phi_strategy(size_t bit_width);
    
    long double evaluate_phi(long double t) const noexcept override;
    const char* strategy_name() const noexcept override { return "polynomial"; }
    double accuracy_bound() const noexcept override;
    bool supports_precision(size_t bit_width) const noexcept override;
    
private:
    size_t bit_width_;
    std::vector<long double> coefficients_;
    
    void load_coefficients();
};

/**
 * @brief Lookup table + interpolation Φ evaluation strategy.
 *
 * Uses precomputed lookup tables with linear or cubic interpolation.
 * Optimized for lower precision types (takum16, takum32).
 */
class lut_phi_strategy : public phi_strategy {
public:
    enum class interpolation_mode { linear, cubic };
    
    lut_phi_strategy(size_t bit_width, interpolation_mode mode);
    
    long double evaluate_phi(long double t) const noexcept override;
    const char* strategy_name() const noexcept override;
    double accuracy_bound() const noexcept override;
    bool supports_precision(size_t bit_width) const noexcept override;
    
private:
    size_t bit_width_;
    interpolation_mode mode_;
    std::vector<long double> lut_values_;
    size_t lut_size_;
    
    void build_lut();
    long double interpolate(long double t, size_t index) const noexcept;
};

/**
 * @brief Hybrid Φ evaluation strategy.
 *
 * Combines coarse LUT with polynomial refinement for optimal
 * balance of speed and accuracy.
 */
class hybrid_phi_strategy : public phi_strategy {
public:
    hybrid_phi_strategy(size_t bit_width, size_t coarse_lut_size = 256);
    
    long double evaluate_phi(long double t) const noexcept override;
    const char* strategy_name() const noexcept override { return "hybrid"; }
    double accuracy_bound() const noexcept override;
    bool supports_precision(size_t bit_width) const noexcept override;
    
private:
    size_t bit_width_;
    size_t coarse_lut_size_;
    std::vector<long double> coarse_lut_;
    std::vector<long double> poly_coeffs_;
    
    void build_coarse_lut();
    void load_polynomial_coefficients();
};

/**
 * @brief Main arithmetic engine with configurable Φ strategy.
 *
 * This class provides the core arithmetic operations (addition, subtraction)
 * while delegating Φ evaluation to the configured strategy. The design
 * enables runtime strategy selection and easy benchmarking.
 */
template<size_t N>
class arithmetic_engine {
public:
    using storage_type = typename core::storage_traits<N>::type;
    using encoder_type = core::encoder<N>;
    
    /**
     * @brief Construct with specific Φ evaluation strategy.
     */
    explicit arithmetic_engine(std::unique_ptr<phi_strategy> strategy)
        : phi_strategy_(std::move(strategy)) {}
    
    /**
     * @brief Construct with default strategy for this precision.
     */
    arithmetic_engine() : arithmetic_engine(create_default_strategy()) {}
    
    /**
     * @brief Perform takum addition using configured Φ strategy.
     *
     * This implements the core Phase-4 addition algorithm with the
     * selected Φ evaluation strategy.
     */
    storage_type add(storage_type a_bits, storage_type b_bits) const noexcept {
        // Step 1: NaR propagation
        if (encoder_type::is_nar(a_bits) || encoder_type::is_nar(b_bits)) {
            return encoder_type::nar_pattern();
        }
        
        // Step 2: Extract ℓ values
        long double ell_a = encoder_type::extract_ell(a_bits);
        long double ell_b = encoder_type::extract_ell(b_bits);
        
        // Step 3: Handle special cases
        if (std::isinf(ell_a) || std::isinf(ell_b)) {
            return handle_special_cases(a_bits, b_bits, ell_a, ell_b);
        }
        
        // Step 4: Ensure larger magnitude first for numerical stability
        if (ell_b > ell_a) {
            std::swap(a_bits, b_bits);
            std::swap(ell_a, ell_b);
        }
        
        // Step 5: Extract signs and check for cancellation
        bool sign_a = encoder_type::extract_sign(a_bits);
        bool sign_b = encoder_type::extract_sign(b_bits);
        
        if (sign_a != sign_b) {
            return handle_subtraction(a_bits, b_bits, ell_a, ell_b);
        }
        
        // Step 6: Perform addition using Φ strategy
        return perform_addition_with_phi(a_bits, b_bits, ell_a, ell_b);
    }
    
    /**
     * @brief Perform takum subtraction.
     */
    storage_type subtract(storage_type a_bits, storage_type b_bits) const noexcept {
        // Negate b and add
        storage_type neg_b_bits = negate_bits(b_bits);
        return add(a_bits, neg_b_bits);
    }
    
    /**
     * @brief Get current strategy information.
     */
    const phi_strategy& get_strategy() const noexcept {
        return *phi_strategy_;
    }
    
    /**
     * @brief Replace the current Φ strategy.
     */
    void set_strategy(std::unique_ptr<phi_strategy> new_strategy) {
        phi_strategy_ = std::move(new_strategy);
    }
    
private:
    std::unique_ptr<phi_strategy> phi_strategy_;
    
    static std::unique_ptr<phi_strategy> create_default_strategy() {
        if constexpr (N <= 32) {
            return std::make_unique<lut_phi_strategy>(N, lut_phi_strategy::interpolation_mode::linear);
        } else if constexpr (N <= 64) {
            return std::make_unique<hybrid_phi_strategy>(N);
        } else {
            return std::make_unique<polynomial_phi_strategy>(N);
        }
    }
    
    storage_type handle_special_cases(storage_type a_bits, storage_type b_bits, 
                                    long double ell_a, long double ell_b) const noexcept {
        // Handle zero operands and infinity cases
        if (std::isinf(ell_a) && ell_a < 0) return b_bits; // a is zero
        if (std::isinf(ell_b) && ell_b < 0) return a_bits; // b is zero
        return encoder_type::nar_pattern(); // Other infinities
    }
    
    storage_type handle_subtraction(storage_type a_bits, storage_type b_bits,
                                  long double ell_a, long double ell_b) const noexcept {
        // Check for perfect cancellation
        if (std::abs(ell_a - ell_b) < 1e-15) {
            return storage_type{0}; // Result is zero
        }
        
        // Use Φ strategy for subtraction (similar to addition but with sign handling)
        return perform_subtraction_with_phi(a_bits, b_bits, ell_a, ell_b);
    }
    
    storage_type perform_addition_with_phi(storage_type a_bits, storage_type b_bits,
                                         long double ell_a, long double ell_b) const noexcept {
        // Core Φ-based addition algorithm
        long double ell_diff = ell_b - ell_a;
        long double t = ell_diff / 2.0L;
        
        // Clamp t to valid domain for Φ function
        t = std::max(-0.5L, std::min(t, 0.0L));
        
        // Evaluate Φ using the configured strategy
        long double phi_value = phi_strategy_->evaluate_phi(t);
        
        // Compute result ℓ value
        long double result_ell = ell_a + 2.0L * phi_value;
        
        // Convert back to takum encoding
        double result_value = std::exp(result_ell / 2.0L);
        bool result_sign = encoder_type::extract_sign(a_bits); // Same sign as both operands
        
        if (result_sign) result_value = -result_value;
        
        return encoder_type::encode(result_value);
    }
    
    storage_type perform_subtraction_with_phi(storage_type a_bits, storage_type b_bits,
                                            long double ell_a, long double ell_b) const noexcept {
        // Similar to addition but with different sign handling
        // Implementation would follow the subtraction formula using Φ
        // For now, fall back to double arithmetic
        double val_a = encoder_type::decode(a_bits);
        double val_b = encoder_type::decode(b_bits);
        return encoder_type::encode(val_a - val_b);
    }
    
    storage_type negate_bits(storage_type bits) const noexcept {
        if (encoder_type::is_nar(bits)) return bits;
        
        if constexpr (core::storage_traits<N>::is_single_word) {
            // Flip sign bit
            return bits ^ (storage_type{1} << (N - 1));
        } else {
            storage_type result = bits;
            result[core::storage_traits<N>::word_count - 1] ^= 
                uint64_t{1} << ((N - 1) % 64);
            return result;
        }
    }
};

} // namespace arithmetic
} // namespace takum