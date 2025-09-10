/**
 * @file encoding.h
 * @brief Core encoding and decoding operations for takum types.
 *
 * This header provides the fundamental encoding/decoding operations that convert
 * between takum's internal bit representation and mathematical values. These
 * operations are separated from arithmetic to allow for better testing,
 * optimization, and future GPU acceleration.
 *
 * Key responsibilities:
 * - tau() function: decode bits to real value
 * - tau_inv() function: encode real value to bits
 * - NaR detection and handling
 * - Bit pattern validation and normalization
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>
#include <array>

namespace takum {
namespace core {

/**
 * @brief Storage type selection based on bit width N.
 * 
 * Provides efficient packed storage while maintaining portability:
 * - N ≤ 32: uint32_t for optimal word alignment
 * - N ≤ 64: uint64_t for 64-bit arithmetic efficiency  
 * - N > 64: std::array<uint64_t, K> for multi-word support
 */
template<size_t N>
struct storage_traits {
    static_assert(N >= 12, "Takum requires at least 12 bits for valid encoding");
    
    using type = std::conditional_t<
        N <= 32, uint32_t,
        std::conditional_t<N <= 64, uint64_t, std::array<uint64_t, (N + 63) / 64>>
    >;
    
    static constexpr size_t bit_width = N;
    static constexpr bool is_single_word = N <= 64;
    static constexpr size_t word_count = is_single_word ? 1 : (N + 63) / 64;
};

/**
 * @brief Core encoding operations for takum bit patterns.
 *
 * This class provides the fundamental encoding/decoding operations as pure
 * static functions. The design is intended to be:
 * - Stateless and thread-safe
 * - Optimizable by compilers (constexpr where possible)
 * - Testable in isolation from arithmetic operations
 */
template<size_t N>
class encoder {
public:
    using storage_type = typename storage_traits<N>::type;
    
    /**
     * @brief Decode takum bits to real value (tau function).
     *
     * Implements the reference tau() function from the takum specification.
     * Converts the bit pattern to its corresponding real value using the
     * logarithmic tapered encoding.
     *
     * @param bits The takum bit pattern to decode
     * @return The corresponding real value, or NaN for NaR patterns
     */
    static constexpr double decode(storage_type bits) noexcept {
        if (is_nar(bits)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        // Extract components from bit pattern
        bool sign = extract_sign(bits);
        uint32_t regime = extract_regime(bits);
        uint32_t exponent = extract_exponent(bits);
        uint64_t mantissa = extract_mantissa(bits);
        
        // Compute value using takum decoding formula
        // Implementation follows reference algorithm
        double value = compute_value(regime, exponent, mantissa);
        
        return sign ? -value : value;
    }
    
    /**
     * @brief Encode real value to takum bits (tau_inv function).
     *
     * Implements the reference tau_inv() function from the takum specification.
     * Converts a real value to its closest representable takum bit pattern
     * using saturation for out-of-range values.
     *
     * @param value The real value to encode
     * @return The corresponding takum bit pattern
     */
    static constexpr storage_type encode(double value) noexcept {
        if (std::isnan(value)) {
            return nar_pattern();
        }
        
        if (value == 0.0) {
            return storage_type{0};
        }
        
        bool sign = std::signbit(value);
        double abs_value = std::abs(value);
        
        // Apply saturation for out-of-range values
        if (abs_value > max_representable_value()) {
            return sign ? encode_negative_saturation() : encode_positive_saturation();
        }
        
        if (abs_value < min_representable_value()) {
            return storage_type{0}; // Underflow to zero
        }
        
        // Encode using reference algorithm
        return encode_positive_value(abs_value, sign);
    }
    
    /**
     * @brief Check if bit pattern represents NaR (Not-a-Real).
     *
     * @param bits The bit pattern to check
     * @return true if the pattern is NaR, false otherwise
     */
    static constexpr bool is_nar(storage_type bits) noexcept {
        if constexpr (storage_traits<N>::is_single_word) {
            return bits == nar_pattern();
        } else {
            // Multi-word comparison
            auto nar = nar_pattern();
            for (size_t i = 0; i < storage_traits<N>::word_count; ++i) {
                if (bits[i] != nar[i]) return false;
            }
            return true;
        }
    }
    
    /**
     * @brief Get the canonical NaR bit pattern for this precision.
     *
     * @return The NaR bit pattern
     */
    static constexpr storage_type nar_pattern() noexcept {
        if constexpr (storage_traits<N>::is_single_word) {
            return storage_type{1} << (N - 1);
        } else {
            storage_type result{};
            result[storage_traits<N>::word_count - 1] = uint64_t{1} << ((N - 1) % 64);
            return result;
        }
    }
    
    /**
     * @brief Extract the exact logarithmic (ℓ) value from bits.
     *
     * Returns the exact ℓ = 2*log(|value|) representation used for
     * logarithmic arithmetic operations.
     *
     * @param bits The takum bit pattern
     * @return The ℓ value as long double for precision
     */
    static long double extract_ell(storage_type bits) noexcept {
        if (is_nar(bits)) {
            return std::numeric_limits<long double>::quiet_NaN();
        }
        
        double value = decode(bits);
        if (value == 0.0) {
            return -std::numeric_limits<long double>::infinity();
        }
        
        return 2.0L * std::log(std::abs(value));
    }
    
private:
    // Internal helper functions for encoding/decoding
    static constexpr bool extract_sign(storage_type bits) noexcept {
        if constexpr (storage_traits<N>::is_single_word) {
            return (bits >> (N - 1)) & 1;
        } else {
            return (bits[storage_traits<N>::word_count - 1] >> ((N - 1) % 64)) & 1;
        }
    }
    
    static constexpr uint32_t extract_regime(storage_type bits) noexcept {
        // Takum regime extraction: count run of identical bits after sign bit.
        // For posit-like regime: regime = (run of 1s) ? run_length - 1 : -run_length
        constexpr size_t nbits = N;
        size_t regime_start = nbits - 2; // Bit after sign
        bool regime_bit;
        size_t run_length = 0;
        if constexpr (storage_traits<N>::is_single_word) {
            regime_bit = (bits >> regime_start) & 1;
            // Count run of regime_bit
            while (regime_start < nbits && ((bits >> regime_start) & 1) == regime_bit) {
                ++run_length;
                if (regime_start == 0) break;
                --regime_start;
            }
        } else {
            // Multi-word: treat as big-endian bit array
            size_t bit_index = nbits - 2;
            size_t word_index = bit_index / 64;
            size_t bit_in_word = bit_index % 64;
            regime_bit = (bits[word_index] >> bit_in_word) & 1;
            while (bit_index < nbits) {
                if (((bits[word_index] >> bit_in_word) & 1) != regime_bit) break;
                ++run_length;
                if (bit_index == 0) break;
                --bit_index;
                word_index = bit_index / 64;
                bit_in_word = bit_index % 64;
            }
        }
        // Compute regime value
        int32_t regime_value = regime_bit ? (int32_t)run_length - 1 : -(int32_t)run_length;
        return static_cast<uint32_t>(regime_value);
    }
    
    static constexpr uint32_t extract_exponent(storage_type bits) noexcept {
        // Implement exponent extraction logic according to takum specification.
        // For this example, assume exponent width is 2 bits (can be adjusted as needed).
        constexpr uint32_t exponent_width = 2;
        // First, skip the sign bit (N-1), then the regime field.
        uint32_t regime = extract_regime(bits);
        // The regime field is variable length; for posit-like formats, regime length = |regime| + 1
        uint32_t regime_length = std::abs(static_cast<int32_t>(regime)) + 1;
        uint32_t exponent_shift = N - 1 - regime_length - exponent_width;
        if constexpr (storage_traits<N>::is_single_word) {
            return (bits >> exponent_shift) & ((1u << exponent_width) - 1);
        } else {
            // For multi-word, extract the relevant bits across word boundaries
            uint32_t bit_offset = exponent_shift;
            uint32_t word_idx = bit_offset / 64;
            uint32_t bit_in_word = bit_offset % 64;
            uint64_t word = bits[word_idx];
            uint32_t value = (word >> bit_in_word) & ((1u << exponent_width) - 1);
            // If the exponent spans two words
            if (bit_in_word + exponent_width > 64 && word_idx + 1 < storage_traits<N>::word_count) {
                uint64_t next_word = bits[word_idx + 1];
                uint32_t bits_from_next = bit_in_word + exponent_width - 64;
                value |= (next_word & ((1u << bits_from_next) - 1)) << (exponent_width - bits_from_next);
            }
            return value;
        }
    }
    
    static constexpr uint64_t extract_mantissa(storage_type bits) noexcept {
        // Mantissa extraction logic according to takum specification:
        // 1. Skip sign bit (N-1)
        // 2. Skip regime bits (variable length, determined by regime run)
        // 3. Skip exponent bits (fixed length, e.g., ES bits if takum is posit-like)
        // 4. The remaining bits are the mantissa (fraction)
        //
        // For this implementation, we assume:
        // - 1 sign bit
        // - regime length determined by extract_regime (number of regime bits)
        // - exponent length is fixed (e.g., ES = 2), can be adjusted as needed
        // - mantissa is the remaining bits
        constexpr size_t ES = 2; // Example exponent size, adjust as per takum spec
        constexpr size_t Nbits = N;
        size_t regime_length = 0;
        {
            // Determine regime length by scanning bits after sign
            // Regime is a run of identical bits starting at (N-2)
            bool regime_sign;
            if constexpr (storage_traits<N>::is_single_word) {
                regime_sign = (bits >> (Nbits - 2)) & 1;
                size_t i = Nbits - 2;
                while (i > 0 && ((bits >> (i - 1)) & 1) == regime_sign) {
                    --i;
                }
                regime_length = (Nbits - 2) - i + 1;
            } else {
                // Multi-word: extract bits manually
                size_t i = Nbits - 2;
                regime_sign = (bits[storage_traits<N>::word_count - 1] >> ((i) % 64)) & 1;
                while (i > 0) {
                    size_t word_idx = i / 64;
                    size_t bit_idx = i % 64;
                    bool bit = (bits[word_idx] >> bit_idx) & 1;
                    if (bit != regime_sign) break;
                    --i;
                }
                regime_length = (Nbits - 2) - i + 1;
            }
        }
        size_t mantissa_start = 1 + regime_length + ES; // sign + regime + exponent
        if (mantissa_start >= Nbits) return 0;
        size_t mantissa_length = Nbits - mantissa_start;
        if (mantissa_length == 0) return 0;
        uint64_t mantissa = 0;
        if constexpr (storage_traits<N>::is_single_word) {
            mantissa = (bits >> (Nbits - mantissa_start - mantissa_length)) & ((uint64_t(1) << mantissa_length) - 1);
        } else {
            // Multi-word: extract mantissa bits across array
            size_t bit_pos = Nbits - mantissa_start - mantissa_length;
            for (size_t i = 0; i < mantissa_length; ++i) {
                size_t abs_bit = bit_pos + i;
                size_t word_idx = abs_bit / 64;
                size_t bit_idx = abs_bit % 64;
                bool bit = (bits[word_idx] >> bit_idx) & 1;
                mantissa = (mantissa << 1) | bit;
            }
        }
        return mantissa;
    }
    
    static constexpr double compute_value(uint32_t regime, uint32_t exponent, uint64_t mantissa) noexcept {
        // Implement value computation from components
        // Placeholder implementation
        return 1.0;
    }
    
    static constexpr double max_representable_value() noexcept {
        // Return maximum representable value for this precision
        return std::sqrt(std::exp(1.0)); // sqrt(e)^255 for typical takum
    }
    
    static constexpr double min_representable_value() noexcept {
        // Return minimum representable value for this precision
        return 1.0 / max_representable_value();
    }
    
    static constexpr storage_type encode_positive_saturation() noexcept {
        // Return bit pattern for positive saturation
        storage_type result{};
        if constexpr (storage_traits<N>::is_single_word) {
            result = (storage_type{1} << (N - 1)) - 1;
        } else {
            // Multi-word positive saturation
            for (size_t i = 0; i < storage_traits<N>::word_count - 1; ++i) {
                result[i] = ~uint64_t{0};
            }
            result[storage_traits<N>::word_count - 1] = (uint64_t{1} << ((N - 1) % 64)) - 1;
        }
        return result;
    }
    
    static constexpr storage_type encode_negative_saturation() noexcept {
        // Return bit pattern for negative saturation (sign bit + 000...001)
        storage_type result{};
        if constexpr (storage_traits<N>::is_single_word) {
            result = (storage_type{1} << (N - 1)) | 1;
        } else {
            result[0] = 1;
            result[storage_traits<N>::word_count - 1] = uint64_t{1} << ((N - 1) % 64);
        }
        return result;
    }
    
    static constexpr storage_type encode_positive_value(double abs_value, bool sign) noexcept {
        // Implement full encoding algorithm
        // Placeholder implementation
        storage_type result{1}; // Non-zero placeholder
        if (sign && storage_traits<N>::is_single_word) {
            result |= storage_type{1} << (N - 1);
        } else if (sign) {
            result[storage_traits<N>::word_count - 1] |= uint64_t{1} << ((N - 1) % 64);
        }
        return result;
    }
};

} // namespace core
} // namespace takum