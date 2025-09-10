#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <bitset>
#include <type_traits>
#include <bit>
#include <optional>
#if __cplusplus >= 202302L
#include <expected>
#else
#include <variant>  // Fallback shim if needed, but use optional for now
#endif

#include "takum/internal/ref/tau_ref.h"
#include <cmath>
#include <limits>
#include <algorithm>

/**
 * @file core.h
 * @brief Core definitions for the Takum numeric type family.
 *
 * This header provides the parametric fixed-bit-width "takum" type template
 * that represents a logarithmic-like numeric encoding (Phase2 reference
 * implementation). It also provides small helpers and an error type used by
 * the optional/expected-style accessors.
 *
 * The implementation intentionally documents places that are placeholders
 * for future multi-word support.
 */

/**
 * @namespace takum
 * @brief Core namespace for the Takum logarithmic numeric format library.
 *
 * This namespace contains the main `takum<N>` template class and supporting
 * utilities for working with tapered logarithmic number formats. The library
 * provides a parametric implementation supporting bit widths from 2 to 256 bits.
 *
 * Key components:
 * - `takum<N>`: The main numeric type template
 * - `takum_error`: Error type for safe operations  
 * - `takum_floating_point`: Concept for takum-like types
 * - Encoding/decoding functions following the reference specification
 *
 * @note All operations handle the special NaR (Not-a-Real) value following
 *       the specification's propagation rules.
 */
namespace takum {

/**
 * @struct takum_error
 * @brief Lightweight error value returned by expected/optional helpers.
 *
 * Used to indicate domain/representation errors when converting or operating
 * on `takum` values. The `message` pointer is optional and may be nullptr.
 */
struct takum_error {
    /// Broad classification of the error.
    enum class Kind { DomainError, Overflow, Underflow, InvalidOperation, Inexact, Internal } kind;
    /// Optional NUL-terminated explanatory string.
    const char* message = nullptr;
};

// Concept mirroring std::floating_point for takum<N> (arithmetic disabled until ready)
template <typename T>
/**
 * @concept takum_floating_point
 * @brief Minimal concept describing the takum-like public API used by the
 * library and tests.
 *
 * Types satisfying this concept must provide comparable semantics, a
 * conversion to host `double`, and a way to test for the special `NaR`
 * (Not-a-Real) value.
 *
 * This is intentionally lightweight — it mirrors the methods used in tests
 * and utilities.
 *
 * The type must be a complete type (sizeof(T) > 0).
 */
concept takum_floating_point = requires(T t) {
    { t < t } -> std::convertible_to<bool>;
    { t == t } -> std::convertible_to<bool>;
    { t.to_double() } -> std::convertible_to<double>;
    { t.is_nar() } -> std::convertible_to<bool>;
} && sizeof(T) > 0; // Ensure complete type

template <size_t N>
/**
 * @brief Configurable-width Takum numeric type.
 *
 * The `takum<N>` template provides an N-bit logarithmic-like numeric value
 * with a special NaR (Not-a-Real) encoding. The implementation uses an
 * underlying integer or array-of-uint64_t storage depending on `N`.
 *
 * @tparam N Bit width in bits; supported range is 2..256 (static_assert enforces this).
 */
struct takum {
    static_assert(N >= 2 && N <= 256, "takum: supported bit widths 2..256 for now");

    /// Underlying storage type chosen based on the width `N`.
    using storage_t = std::conditional_t<(N <= 32), uint32_t,
                      std::conditional_t<(N <= 64), uint64_t, std::array<uint64_t, (N+63)/64>>>;

    /// Raw storage containing the N-bit pattern (valid bits are in the low N bits).
    storage_t storage{};

    /**
     * @brief Default constructs a zero-valued `takum`.
     *
     * The default is noexcept and initializes the storage to the zero pattern.
     */
    constexpr takum() noexcept = default;

    /**
     * @brief Construct the canonical NaR (Not-a-Real) pattern.
     *
     * @return A `takum` value representing NaR.
     * @note The canonical NaR is represented by the sign bit set and all
     *       other bits zero in this reference implementation.
     */
    static takum nar() noexcept {
        takum t{};
        if constexpr (N <= 64) {
            uint64_t pat = 1ULL << (N - 1);  // Only sign bit set for NaR per spec
            t.storage = static_cast<storage_t>(pat);
        } else {
            // For large N, set only the sign bit (MSB)
            size_t msb_word = (N - 1) / 64;
            size_t msb_bit = (N - 1) % 64;
            t.storage[msb_word] = 1ULL << msb_bit;
            // Ensure other words are zero (already default)
        }
        return t;
    }

    /**
     * @brief Test whether the value is NaR.
     *
     * @return true if this value is the canonical NaR pattern.
     */
    bool is_nar() const noexcept {
        if constexpr (N <= 64) {
            uint64_t w = uint64_t(storage);
            uint64_t pat = 1ULL << (N - 1);  // Only sign bit set for NaR
            return w == pat;
        } else {
            // Check only sign bit (MSB) set, all else zero
            size_t msb_word = (N - 1) / 64;
            size_t msb_bit = (N - 1) % 64;
            uint64_t expected = 1ULL << msb_bit;
            if (storage[msb_word] != expected) return false;
            for (size_t i = 0; i < storage.size(); ++i) {
                if (i != msb_word && storage[i] != 0) return false;
            }
            return true;
        }
    }

    /**
     * @name Comparison operators
     * These operators implement a total order where NaR is considered the
     * smallest element and all real values are ordered monotonically.
     */
    //@{
    bool operator==(const takum& other) const noexcept {
        if (is_nar() && other.is_nar()) return true;
        if (is_nar() || other.is_nar()) return false;
        return storage == other.storage;  // Bitwise equal for reals
    }

    /**
     * @brief Total-order less-than comparison.
     *
     * NaR is ordered below any real value. For real values the comparison is
     * implemented as a signed integer comparison of the N-bit pattern (with
     * sign extension) for monotonicity.
     */
    bool operator<(const takum& other) const noexcept {
        if (is_nar() || other.is_nar()) {
            return is_nar() && !other.is_nar();
        }
        // Both real: signed integer compare with sign extension for N bits
        if constexpr (N > 64) {
            // Multi-word signed compare: perform sign-aware lexicographic compare
            // Words are stored little-endian (word 0 is least-significant). Determine
            // the index and bit of the sign and perform comparison starting from
            // the most-significant word.
            size_t words = storage.size();
            // compute msb word index
            size_t msb_word = (N - 1) / 64;
            // Sign bits assumed stored in msb_word at position msb_bit
            uint64_t self_msw = storage[msb_word];
            uint64_t other_msw = other.storage[msb_word];
            // Signed compare by lexicographic order with top word sign extension
            for (size_t wi = msb_word + 1; wi-- > 0;) {
                uint64_t a = storage[wi];
                uint64_t b = other.storage[wi];
                if (a == b) {
                    if (wi == 0) break;
                    continue;
                }
                // For the top word, perform signed comparison limited to used bits
                if (wi == msb_word) {
                    int used_bits = ((N - 1) % 64) + 1;
                    uint64_t mask = (used_bits == 64) ? ~0ULL : ((1ULL << used_bits) - 1ULL);
                    uint64_t a_top = a & mask;
                    uint64_t b_top = b & mask;
                    // sign-extend to 128-bit conceptual: compare as signed of used_bits
                    int64_t a_signed = static_cast<int64_t>( (a_top << (64 - used_bits)) ) >> (64 - used_bits);
                    int64_t b_signed = static_cast<int64_t>( (b_top << (64 - used_bits)) ) >> (64 - used_bits);
                    return a_signed < b_signed;
                }
                // full-word unsigned comparison suffices for lower words
                return a < b ? true : false;
            }
            return false; // equal
        } else {
            // Explicit sign extension for monotonic comparison (Gustafson criterion 6).
            // Treat the low N bits as a signed integer by shifting to MSB and arithmetic right shift.
            int64_t self_signed = static_cast<int64_t>(static_cast<uint64_t>(storage) << (64 - N)) >> (64 - N);
            int64_t other_signed = static_cast<int64_t>(static_cast<uint64_t>(other.storage) << (64 - N)) >> (64 - N);
            return self_signed < other_signed;
        }
    }

    bool operator<=(const takum& other) const noexcept { return !(other < *this); }
    bool operator>(const takum& other) const noexcept { return other < *this; }
    bool operator>=(const takum& other) const noexcept { return !(*this < other); }
    bool operator!=(const takum& other) const noexcept { return !(*this == other); }
    //@}

    /**
     * @brief Bitwise complement of the N-bit pattern (masked to N bits).
     */
    takum operator~() const noexcept {
        takum res;
        if constexpr (std::is_integral_v<storage_t>) {
            uint64_t w = ~uint64_t(storage);
            w &= nbit_mask();
            res.storage = static_cast<storage_t>(w);
        } else {
            for (size_t i = 0; i < res.storage.size(); ++i) res.storage[i] = ~storage[i];
            mask_to_N(res.storage);
        }
        return res;
    }



    /**
     * @brief Smallest positive representable non-zero value (pattern with LSB=1).
     */
    static takum minpos() noexcept {
        takum r;
        if constexpr (std::is_integral_v<storage_t>) {
            r.storage = 1; // LSB set → minimal magnitude
        } else {
            r.storage.fill(0);
            r.storage[0] = 1;
        }
        return r;
    }

    /**
     * @brief Test whether the numeric sign bit is set (value negative).
     * @return true if the sign bit (MSB) is 1.
     */
    bool signbit() const noexcept {
        if constexpr (std::is_integral_v<storage_t>) {
            return (storage >> (N - 1)) & 1;
        } else {
            size_t msb_word = (N - 1) / 64;
            size_t msb_bit  = (N - 1) % 64;
            return (storage[msb_word] >> msb_bit) & 1ULL;
        }
    }

    /**
     * @brief Unary negation: flips the sign bit. NaR is its own negation.
     */
    takum operator-() const noexcept {
        if (is_nar()) return *this; // NaR is its own negation.
        
        takum res = *this;
        if constexpr (N <= 64) {
            // XOR with a mask for the sign bit (bit N-1)
            res.storage ^= (storage_t(1) << (N - 1));
        } else {
            // For multi-word, XOR the sign bit in the highest word
            size_t msb_word = (N - 1) / 64;
            size_t msb_bit_in_word = (N - 1) % 64;
            res.storage[msb_word] ^= (uint64_t(1) << msb_bit_in_word);
        }
        return res;
    }

    /**
     * @brief Compute the reciprocal in the reference codec.
     *
     * Returns NaR for NaR or zero input. This implements the simple bitwise
     * inverse-plus-one rule used by the Phase2 reference codec.
     */
    takum reciprocal() const noexcept {
        if (is_nar()) return nar();

        // Zero → NaR
        if constexpr (std::is_integral_v<storage_t>) {
            if (storage == 0) return nar();
        } else {
            bool all_zero = true;
            for (auto w : storage) if (w != 0) { all_zero = false; break; }
            if (all_zero) return nar();
        }

        takum res;

        if constexpr (N <= 64) {
            uint64_t bits = uint64_t(storage);

            // Reciprocal core: ~bits + 1 (mod 2^N)
            bits = (~bits + 1ULL) & ((N == 64) ? ~0ULL : ((1ULL << N) - 1ULL));

            // *** Fix: flip sign bit back ***
            bits ^= (1ULL << (N - 1));

            res.storage = static_cast<storage_t>(bits);
        } else {
            // multiword version
            for (size_t i = 0; i < res.storage.size(); ++i) res.storage[i] = ~storage[i];

            // +1 with carry
            bool carry = true;
            for (size_t i = 0; i < res.storage.size() && carry; ++i) {
                uint64_t prev = res.storage[i];
                res.storage[i] = prev + 1ULL;
                carry = (res.storage[i] == 0ULL);
            }

            // mask top word to N bits
            size_t msb_word = (N - 1) / 64;
            size_t used_bits = (N - 1) % 64 + 1;
            uint64_t mask = (used_bits == 64) ? ~0ULL : ((1ULL << used_bits) - 1ULL);
            res.storage[msb_word] &= mask;
            for (size_t i = msb_word + 1; i < res.storage.size(); ++i) res.storage[i] = 0;

            // *** Fix: flip sign bit back ***
            res.storage[msb_word] ^= (1ULL << ((N - 1) % 64));
        }

        return res;
    }

    /**
     * @brief Return raw storage bits.
     * @return The underlying storage value(s) containing the N-bit pattern.
     */
    storage_t raw_bits() const noexcept { return storage; }

    /**
     * @brief Create a `takum` from raw storage bits without validation.
     * @param bits Raw storage bits (low N bits are used).
     */
    static takum from_raw_bits(storage_t bits) noexcept {
        takum t{};
        t.storage = bits;
        return t;
    }

    

#if __cplusplus >= 202302L
    /**
     * @brief Convert to std::expected, returning unexpected on NaR.
     */
    std::expected<takum, takum_error> to_expected() const noexcept {
        if (is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR"});
        return *this;
    }
#else
    /**
     * @brief Convert to optional (nullopt if NaR) for pre-C++23 builds.
     */
    std::optional<takum> to_expected() const noexcept {
        if (is_nar()) return std::nullopt;
        return *this;
    }
#endif

    /**
     * @brief Return a bitset view useful for debugging and tests.
     * @return std::bitset<N> where bit 0 is the least-significant stored bit.
     */
    std::bitset<N> debug_view() const noexcept {
        std::bitset<N> b;
        if constexpr (N <= 64) {
            uint64_t w = uint64_t(storage);
            for (size_t i = 0; i < N; ++i) if (w & (uint64_t(1) << i)) b.set(i);
        } else {
            // pack from storage words (little-endian words)
            size_t pos = 0;
            for (uint64_t word : storage) {
                for (size_t i = 0; i < 64 && pos < N; ++i, ++pos) if (word & (uint64_t(1) << i)) b.set(pos);
            }
        }
        return b;
    }

    /**
     * @brief Test whether the value represents zero.
     * @return true if all bits in the storage are zero
     * @note Zero is represented by all bits being zero in the takum format
     */
    bool is_zero() const noexcept {
        if constexpr (N <= 64) return uint64_t(storage) == 0ULL;
        else {
            for (size_t i = 0; i < storage.size(); ++i) if (storage[i] != 0ULL) return false;
            return true;
        }
    }

    /**
     * @brief Convert a host `double` into the reference takum bit pattern.
     * @param x Input double.
     */
    explicit takum(double x) noexcept {
        storage = takum::encode_from_double(x);
    }

    /**
     * @brief Convert this takum value to host `double` using the reference codec.
     * @return Approximate `double` value; NaR converts to quiet NaN.
     */
    double to_double() const noexcept {
        return takum::decode_to_double(storage);
    }

    /**
     * @brief Extract the exact internal logarithmic value ℓ = (-1)^S * (c + m).
     *
     * Useful for diagnostics: returns the characteristic+mantissa with sign
     * applied. For NaR this returns NaN.
     */
    double get_exact_ell() const noexcept {
        // Use decode helper that works for any N; return NaN for NaR
        // For very large N we still provide a double diagnostic only.
        if (is_zero()) return 0.0;
        // Use high-precision decode where available for N<=64
        if constexpr (N <= 64) {
            uint64_t bits = uint64_t(storage);
            // NaR check
            bool S = (bits >> (N - 1)) & 1ULL;
            uint64_t lower = bits & ((1ULL << (N - 1)) - 1ULL);
            if (S && lower == 0) return std::numeric_limits<double>::quiet_NaN();
        } else {
            // Check NaR pattern via is_nar()
            if (is_nar()) return std::numeric_limits<double>::quiet_NaN();
        }
        // Fallback: decode to double and then extract ell via log
        double v = to_double();
        if (!std::isfinite(v)) return std::numeric_limits<double>::quiet_NaN();
        long double abs_v = std::fabsl(v);
        long double ell = 2.0L * std::logl(abs_v);
        return static_cast<double>((v < 0) ? -ell : ell);
    }

    /**
     * @brief Generate the maximum finite positive storage pattern for this format.
     *
     * Constructs the bit pattern representing the largest representable positive
     * finite value in the takum<N> format. Used for determining dynamic range
     * and testing boundary conditions.
     *
     * @return uint64_t containing the maximum finite positive bit pattern
     * @note For N>64, returns placeholder value 0; full multi-word support pending
     */
    static uint64_t max_finite_storage() noexcept {
        if constexpr (N > 64) {
            // Placeholder for large N; full multi-word packing later
            return 0ULL;
        }
        size_t max_r = std::min<size_t>(7, N - 5);
        bool S = false;  // positive
        bool D = true;
        uint32_t R = static_cast<uint32_t>(max_r);  // r = R since D=1
        size_t p = N - 5 - max_r;
        uint64_t c_bits = (max_r == 0) ? 0ULL : ((1ULL << max_r) - 1ULL);
        uint64_t packed = (static_cast<uint64_t>(S) << (N - 1)) |
                          (static_cast<uint64_t>(D) << (N - 2)) |
                          (static_cast<uint64_t>(R) << (N - 5)) |
                          (c_bits << p);
        uint64_t m_max = (p > 0) ? ((1ULL << p) - 1ULL) : 0ULL;
        packed |= m_max;
        return packed;
    }
    
    /**
     * @brief Get the maximum representable ℓ value for this takum format.
     *
     * Returns the logarithmic value ℓ corresponding to the maximum finite
     * representable number in this format. Used for determining dynamic range.
     *
     * @return long double The maximum ℓ value (approximately ±255 for large N)
     * @note For N>64, uses specification bound of ±255
     * @note For smaller N, computed from the actual maximum finite pattern
     */
    static long double max_ell() noexcept {
        if constexpr (N > 64) {
            // For large N use the spec dynamic range bound: |ell| <= 255
            // (the tapered format limits ℓ to roughly ±255 in the reference spec).
            return 255.0L;
        } else {
            uint64_t bits = max_finite_storage();
            takum temp{};
            temp.storage = static_cast<storage_t>(bits);
            return static_cast<long double>(temp.get_exact_ell());
        }
    }

    /**
     * @brief Encode a host `double` into the takum bit pattern using the
     * reference encoding.
     *
     * This function packs S,D,R,C,M fields per the Phase2 reference
     * specification. Special cases: zero maps to zero; non-finite maps to NaR.
     *
     * For N<=64 the returned value is stored in a single integer; for larger
     * N the returned value is the multi-word storage_t.
     */
    static storage_t encode_from_double(double x) noexcept {
        if constexpr (N <= 64) {
            // Small-width path: build uint64_t then widen/copy
            uint64_t small = encode_from_double_u64(x);
            return static_cast<storage_t>(small);
        } else {
            // Multi-word: write the bit fields across the words using helpers.
            storage_t out{};
            out.fill(0);
            // If x is zero or non-finite the small path already encodes it in low bits
            // but we must still place fields at correct offsets across words.
            if (x == 0.0) return out;
            if (!std::isfinite(x)) { out = takum::nar().storage; return out; }

            bool S = std::signbit(x);
            long double abs_x = std::fabsl(x);
            long double ell = 2.0L * std::logl(abs_x);

            long double clamp_pos = max_ell();
            // Per spec: values outside representable dynamic range map to NaR
            if (ell > clamp_pos || ell < -clamp_pos) {
                out = takum::nar().storage;
                return out;
            }

            int64_t c = static_cast<int64_t>(std::floorl(ell));
            bool D = (c >= 0);
            int64_t abs_c = D ? c : -c;
            uint32_t r = (abs_c != 0)
                ? static_cast<uint32_t>(std::floorl(std::log2l(D ? abs_c + 1 : abs_c)))
                : 0;
            r = std::min<uint32_t>(7, r);
            uint32_t R = D ? r : (7U - r);

            uint64_t c_bits = 0ULL;
            if (r != 0) {
                if (D) c_bits = static_cast<uint64_t>(c - ((1ULL << r) - 1ULL));
                else c_bits = static_cast<uint64_t>(c + ((1ULL << (r+1)) - 1ULL));
            }

            long double m = ell - static_cast<long double>(c);
            if (m < 0.0L) m = 0.0L;
            if (m >= 1.0L) m = 0.999999L;

            size_t p = N - 5 - static_cast<size_t>(r);

            // Helper lambdas for multi-word bit writes
            auto write_bit = [&](storage_t &dst, size_t bit_index, bool val) {
                size_t wi = bit_index / 64;
                size_t bi = bit_index % 64;
                if constexpr (N <= 64) {
                    // not used here
                } else {
                    if (val) dst[wi] |= (1ULL << bi);
                    else dst[wi] &= ~(1ULL << bi);
                }
            };

            auto write_u64_at = [&](storage_t &dst, size_t bit_index, uint64_t value, size_t len) {
                // Write up to 64 bits of `value` starting at bit_index (LSB-first), crossing words.
                size_t remaining = len;
                size_t pos = 0;
                while (remaining > 0) {
                    size_t wi = bit_index / 64;
                    size_t bi = bit_index % 64;
                    size_t space = 64 - bi;
                    size_t take = std::min<size_t>(remaining, space);
                    uint64_t mask = (take == 64) ? ~0ULL : ((1ULL << take) - 1ULL);
                    uint64_t chunk = (value >> pos) & mask;
                    // clear destination bits then set
                    uint64_t dst_mask = ~(mask << bi);
                    dst[wi] &= dst_mask;
                    dst[wi] |= (chunk << bi);
                    bit_index += take;
                    pos += take;
                    remaining -= take;
                }
            };

            // Place S at N-1
            write_bit(out, N - 1, S);
            // D at N-2
            write_bit(out, N - 2, D);
            // R: 3 bits at positions [N-5 .. N-3]
            write_u64_at(out, N - 5, static_cast<uint64_t>(R), 3);
            // C bits: r bits at offset p
            if (r > 0) write_u64_at(out, p, c_bits, static_cast<size_t>(r));
            // M bits: write p bits representing fractional part m; generate bits sequentially
            long double frac = m;
            for (size_t i = 0; i < p; ++i) {
                frac *= 2.0L;
                bool bit = (frac >= 1.0L);
                if (bit) frac -= 1.0L;
                write_bit(out, i, bit);
            }

            // Mask top bits just in case
            mask_to_N(out);
            return out;
        }
    }

    /**
     * @brief Construct takum storage directly from sign and logarithmic value ℓ.
     *
     * This avoids converting via host double when the logarithmic value is
     * already known (Phase‑4 native encoder). Returns NaR for out-of-range ℓ
     * (|ℓ| > max_ell()) or when ℓ is NaN.
     */
    static takum from_ell(bool S, long double ell_ld) noexcept {
        // Handle NaR/NaN
        if (!std::isfinite((double)ell_ld)) return takum::nar();

        long double clamp_pos = max_ell();
        if (ell_ld > clamp_pos || ell_ld < -clamp_pos) return takum::nar();

        // Zero is represented by an all-zero pattern; interpret very small
        // negative ell as zero (user-level code should handle exact zeros).
        // Here, if ell is extremely negative, produce zero.
        if (ell_ld <= -1e300L) {
            return takum{}; // zero
        }

        if constexpr (N <= 64) {
            // Follow encode_from_double_u64 packing but use provided ell
            int64_t c = static_cast<int64_t>(std::floorl(ell_ld));
            bool D = (c >= 0);
            int64_t abs_c = D ? c : -c;
            uint32_t r = (abs_c != 0)
                ? static_cast<uint32_t>(std::floorl(std::log2l(D ? abs_c + 1 : abs_c)))
                : 0;
            r = std::min<uint32_t>(7, r);
            uint32_t R = D ? r : (7U - r);

            uint64_t c_bits = 0ULL;
            if (r != 0) {
                if (D) c_bits = static_cast<uint64_t>(c - ((1ULL << r) - 1ULL));
                else c_bits = static_cast<uint64_t>(c + ((1ULL << (r+1)) - 1ULL));
            }

            long double m = ell_ld - static_cast<long double>(c);
            if (m < 0.0L) m = 0.0L;
            if (m >= 1.0L) m = 0.999999L;

            size_t p = N - 5 - static_cast<size_t>(r);
            uint64_t m_bits = 0ULL;
            if (p > 0 && m > 0.0L) {
                long double m_power = std::ldexpl(1.0L, static_cast<int>(p));
                long double m_scaled_ld = m * m_power;
                m_bits = static_cast<uint64_t>(std::floorl(m_scaled_ld + 0.5L));
                uint64_t max_m = (p >= 64) ? ~0ULL : ((1ULL << p) - 1ULL);
                if (m_bits > max_m) m_bits = max_m;
            }

            uint64_t packed = (static_cast<uint64_t>(S) << (N - 1)) |
                              (static_cast<uint64_t>(D) << (N - 2)) |
                              (static_cast<uint64_t>(R) << (N - 5));
            packed |= (c_bits << p);
            packed |= m_bits;
            takum t{};
            t.storage = static_cast<storage_t>(packed);
            return t;
        } else {
            // Multi-word packing similar to encode_from_double multiword path
            storage_t out{};
            out.fill(0);

            bool D;
            int64_t c;
            {
                c = static_cast<int64_t>(std::floorl(ell_ld));
                D = (c >= 0);
            }
            int64_t abs_c = D ? c : -c;
            uint32_t r = (abs_c != 0)
                ? static_cast<uint32_t>(std::floorl(std::log2l(D ? abs_c + 1 : abs_c)))
                : 0;
            r = std::min<uint32_t>(7, r);
            uint32_t R = D ? r : (7U - r);

            uint64_t c_bits = 0ULL;
            if (r != 0) {
                if (D) c_bits = static_cast<uint64_t>(c - ((1ULL << r) - 1ULL));
                else c_bits = static_cast<uint64_t>(c + ((1ULL << (r+1)) - 1ULL));
            }

            long double m = ell_ld - static_cast<long double>(c);
            if (m < 0.0L) m = 0.0L;
            if (m >= 1.0L) m = 0.999999L;

            size_t p = N - 5 - static_cast<size_t>(r);

            auto write_bit = [&](storage_t &dst, size_t bit_index, bool val) {
                size_t wi = bit_index / 64;
                size_t bi = bit_index % 64;
                if (val) dst[wi] |= (1ULL << bi);
                else dst[wi] &= ~(1ULL << bi);
            };

            auto write_u64_at = [&](storage_t &dst, size_t bit_index, uint64_t value, size_t len) {
                size_t remaining = len;
                size_t pos = 0;
                while (remaining > 0) {
                    size_t wi = bit_index / 64;
                    size_t bi = bit_index % 64;
                    size_t space = 64 - bi;
                    size_t take = std::min<size_t>(remaining, space);
                    uint64_t mask = (take == 64) ? ~0ULL : ((1ULL << take) - 1ULL);
                    uint64_t chunk = (value >> pos) & mask;
                    uint64_t dst_mask = ~(mask << bi);
                    dst[wi] &= dst_mask;
                    dst[wi] |= (chunk << bi);
                    bit_index += take;
                    pos += take;
                    remaining -= take;
                }
            };

            // Place S at N-1
            write_bit(out, N - 1, S);
            // D at N-2
            write_bit(out, N - 2, D);
            // R: 3 bits at positions [N-5 .. N-3]
            write_u64_at(out, N - 5, static_cast<uint64_t>(R), 3);
            // C bits: r bits at offset p
            if (r > 0) write_u64_at(out, p, c_bits, static_cast<size_t>(r));
            // M bits: write p bits representing fractional part m; generate bits sequentially
            long double frac = m;
            for (size_t i = 0; i < p; ++i) {
                frac *= 2.0L;
                bool bit = (frac >= 1.0L);
                if (bit) frac -= 1.0L;
                write_bit(out, i, bit);
            }

            mask_to_N(out);
            takum t{};
            t.storage = out;
            return t;
        }
    }

    /**
     * @brief Helper to encode double to uint64_t for single-word takum formats.
     *
     * Optimized encoding path for N<=64 bit formats. Implements the full
     * S,D,R,C,M field packing per the reference specification equations.
     *
     * @param x Input double value to encode
     * @return uint64_t containing the packed N-bit takum representation
     * @note Only used for N<=64; larger formats use the multi-word encode path
     * @note Implements equations (14)-(24) from the reference specification
     */
    static uint64_t encode_from_double_u64(double x) noexcept {
        if constexpr (N <= 64) {
            if (x == 0.0) return 0ULL; // Zero representation per eq. (24)
            if (!std::isfinite(x)) return static_cast<uint64_t>(takum::nar().storage);  // NaN/Inf → NaR per NaR convention in Def. 2

            bool S = std::signbit(x); // Sign bit per eq. (14)
            long double abs_x = std::fabsl(x);

            long double ell = 2.0L * std::logl(abs_x); // Logarithmic value ℓ = 2 * ln(|x|) for base √e, per eq. (23)

            // Clamp |ℓ| to representable range |ℓ| < 255 per eq. (23)
            long double clamp_pos = max_ell();
            if (ell > clamp_pos) ell = clamp_pos;
            if (ell < -clamp_pos) ell = -clamp_pos;

            // Decompose ℓ into characteristic c = floor(ℓ) and mantissa m = ℓ - c per eq. (19) and (22)
            int64_t c = static_cast<int64_t>(std::floorl(ell));
            bool D = (c >= 0); // Direction bit: positive if c >= 0 per eq. (15)
            int64_t abs_c = D ? c : -c;

            // Regime r per eq. (17): floor(log2(|c| + D))
            uint32_t r = (abs_c != 0)
                ? static_cast<uint32_t>(std::floorl(std::log2l(D ? abs_c + 1 : abs_c)))
                : 0;
            r = std::min<uint32_t>(7, r); // Clamp to max regime 7
            uint32_t R = D ? r : (7U - r); // Regime bits per eq. (16)

            // Characteristic bits C per eq. (18), c per eq. (19)
            uint64_t c_bits = 0ULL;
            if (r != 0) {
                if (D) {
                    c_bits = static_cast<uint64_t>(c - ((1ULL << r) - 1ULL)); // D=1 case
                } else {
                    c_bits = static_cast<uint64_t>(c + ((1ULL << (r+1)) - 1ULL)); // D=0 case
                }
            }

            // Mantissa m = fractional part of ℓ per eq. (22), p = N - 5 - r per eq. (20)
            long double m = ell - static_cast<long double>(c);
            if (m < 0.0L) m = 0.0L;
            if (m >= 1.0L) m = 0.999999L;  // avoid overflow in scaling

            size_t p = N - 5 - static_cast<size_t>(r);
            uint64_t m_bits = 0ULL;
            if (p > 0 && m > 0.0L) {
                long double m_power = std::ldexpl(1.0L, static_cast<int>(p));
                long double m_scaled_ld = m * m_power;
                m_bits = static_cast<uint64_t>(std::floorl(m_scaled_ld + 0.5L)); // Quantize m to p bits
                uint64_t max_m = (p >= 64) ? ~0ULL : ((1ULL << p) - 1ULL);
                if (m_bits > max_m) m_bits = max_m; // Clamp
            }

            // Pack bit fields into storage per Def. 2 bit layout: S D R C M
            uint64_t packed = (static_cast<uint64_t>(S) << (N - 1)) | // Sign
                              (static_cast<uint64_t>(D) << (N - 2)) | // Direction
                              (static_cast<uint64_t>(R) << (N - 5)); // Regime
            packed |= (c_bits << p); // Characteristic
            packed |= m_bits; // Mantissa
            return packed;
        } else {
            // For N>64 this helper is not used; provide a safe fallback
            return 0ULL;
        }
    }

private:
    /**
     * @brief Generate bit mask for N-bit values.
     * @return Mask with low N bits set to 1
     */
    static constexpr uint64_t nbit_mask() noexcept {
        if constexpr (N >= 64) return ~0ULL;
        else return (N == 64 ? ~0ULL : ((1ULL << N) - 1ULL));
    }

    /**
     * @brief Mask storage to exactly N bits, zeroing any excess bits.
     * @param s Storage reference to mask in-place
     */
    static void mask_to_N(storage_t& s) noexcept {
        if constexpr (N <= 64) {
            s = static_cast<storage_t>(uint64_t(s) & nbit_mask());
        } else {
            // Zero bits above N in the top word; lower words untouched.
            size_t msb_word = (N - 1) / 64;
            size_t used_bits_top = ((N - 1) % 64) + 1;
            uint64_t top_mask = (used_bits_top == 64) ? ~0ULL : ((1ULL << used_bits_top) - 1ULL);
            for (size_t i = msb_word + 1; i < s.size(); ++i) s[i] = 0; // safety
            s[msb_word] &= top_mask;
        }
    }

    /**
     * @brief Static helper to decode takum storage to double value.
     *
     * Unpacks the S,D,R,C,M fields per Definition 2 and computes the real value
     * using equation (24) from the specification. Handles both single-word and
     * multi-word storage formats.
     *
     * @param bits_storage The raw storage containing the N-bit takum pattern
     * @return double The decoded real value, or NaN for NaR patterns
     * @note This is the main decoding function used by to_double()
     */
    static double decode_to_double(const storage_t& bits_storage) noexcept {
        // Extract single-word bits for N<=64 to reuse previous logic
        if constexpr (N <= 64) {
            uint64_t bits = static_cast<uint64_t>(bits_storage);
            return decode_u64_to_double(bits);
        } else {
            // Multi-word decode: read fields across words using bit-access helpers.
            auto read_bit = [&](size_t bit_index) -> bool {
                size_t wi = bit_index / 64;
                size_t bi = bit_index % 64;
                return ((bits_storage[wi] >> bi) & 1ULL) != 0ULL;
            };

            auto read_u64_at = [&](size_t bit_index, size_t len) -> uint64_t {
                // Read up to 64 bits starting at bit_index (LSB-first)
                uint64_t acc = 0ULL;
                size_t remaining = len;
                size_t pos = 0;
                while (remaining > 0) {
                    size_t wi = bit_index / 64;
                    size_t bi = bit_index % 64;
                    size_t space = 64 - bi;
                    size_t take = std::min<size_t>(remaining, space);
                    uint64_t mask = (take == 64) ? ~0ULL : ((1ULL << take) - 1ULL);
                    uint64_t chunk = (bits_storage[wi] >> bi) & mask;
                    acc |= (chunk << pos);
                    bit_index += take;
                    pos += take;
                    remaining -= take;
                }
                return acc;
            };

            // Read sign and NaR/zero checks
            bool S = read_bit(N - 1);
            // Read lower (N-1) bits to check NaR or zero
            bool lower_all_zero = true;
            for (size_t i = 0; i < N - 1; ++i) if (read_bit(i)) { lower_all_zero = false; break; }
            if (S && lower_all_zero) return std::numeric_limits<double>::quiet_NaN();
            // If sign==0 and all lower bits zero, this is the zero pattern
            if (!S && lower_all_zero) return 0.0;

            bool D = read_bit(N - 2);
            uint32_t R = static_cast<uint32_t>(read_u64_at(N - 5, 3));
            uint32_t r = D ? R : (7U - R);
            size_t p = N - 5 - r;
            uint64_t c_bits = (r == 0) ? 0ULL : read_u64_at(p, static_cast<size_t>(r));

            int64_t c = 0;
            if (r == 0) c = 0;
            else {
                if (D) c = static_cast<int64_t>(((1ULL << r) - 1ULL) + c_bits);
                else c = static_cast<int64_t>(-((1LL << (r + 1)) ) + 1LL + static_cast<int64_t>(c_bits));
            }

            // Read mantissa bits individually to build fractional value m
            long double m = 0.0L;
            long double twopow = 0.5L;
            for (size_t i = 0; i < p; ++i) {
                if (read_bit(i)) m += twopow;
                twopow *= 0.5L;
            }

            long double ell = static_cast<long double>(c) + m;
            double value_sign = S ? -1.0 : 1.0;
            return value_sign * static_cast<double>(std::expl(ell * 0.5L));
        }
    }

    /**
     * @brief Helper function to decode single-word takum patterns to double.
     *
     * Optimized decode path for N<=64 bit formats. Unpacks S,D,R,C,M fields
     * from a uint64_t and computes the real value per equation (24).
     *
     * @param bits uint64_t containing the N-bit takum pattern
     * @return double The decoded value following the reference specification
     * @note Zero patterns return exactly 0.0, NaR patterns return quiet NaN
     */
    static double decode_u64_to_double(uint64_t bits) noexcept {
        if (bits == 0) return 0.0; // Zero per eq. (24)
        // NaR check: S=1 and D=R=C=M=0 per Def. 2
        bool S = (bits >> (N - 1)) & 1ULL; // Sign per eq. (14)
        uint64_t lower = bits & ((N >= 64) ? ~0ULL : ((1ULL << (N - 1)) - 1ULL));
        if (S && lower == 0) {
            return std::numeric_limits<double>::quiet_NaN(); // NaR per eq. (24)
        }
        // Extract D per eq. (15)
        bool D = (bits >> (N - 2)) & 1ULL;
        // Extract R per eq. (16), compute r per eq. (17)
        uint32_t R = ((bits >> (N - 5)) & 7ULL);
        uint32_t r = D ? R : (7U - R);
        // Extract C per eq. (18), compute c per eq. (19)
        uint64_t c_mask = (r == 0) ? 0ULL : (((1ULL << r) - 1ULL) << (N - 5 - r));
        uint64_t c_bits = (c_mask == 0ULL) ? 0ULL : ((bits & c_mask) >> (N - 5 - r));
        int64_t c = D
            ? (int64_t(((r == 0) ? 0ULL : ((1ULL << r) - 1ULL)) + c_bits)) // D=1 case eq. (19)
            : (int64_t(-((1LL << (r + 1))) + 1LL + static_cast<int64_t>(c_bits))); // D=0 case eq. (19)
        // p per eq. (20)
        size_t p = N - 5 - r;
        // Extract M per eq. (21), compute m per eq. (22)
        uint64_t m_bits = (p == 0) ? 0ULL : (bits & ((1ULL << p) - 1ULL));
        double m = (p > 0) ? (static_cast<double>(m_bits) * std::pow(2.0, -static_cast<int>(p))) : 0.0;

        // ℓ = c + m per eq. (23)
        double ell = static_cast<double>(c) + m;
        // value = (-1)^S * exp(ℓ / 2) = (-1)^S * √e ^ ℓ per eq. (24)
        double value_sign = S ? -1.0 : 1.0;
        return value_sign * std::exp(ell * 0.5);
    }
};

} // namespace takum

/**
 * @namespace std
 * @brief Standard library namespace extended with takum specializations.
 */
namespace std {

/**
 * @brief Specialization of std::numeric_limits for takum<N> types.
 *
 * Provides numeric limits and traits for takum types to integrate with
 * standard library algorithms and utilities. The traits reflect the
 * logarithmic nature and limited precision of the takum format.
 *
 * @tparam N The bit width of the takum format
 * @note Some values are approximations based on the bit width and format
 * @note is_iec559 is false as takum is not IEEE 754 compliant
 */
template <size_t N>
struct numeric_limits<takum::takum<N>> {
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = false;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = true;
    static constexpr bool has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm = denorm_absent;
    static constexpr bool has_denorm_loss = false;
    static constexpr bool is_iec559 = false; // Deprecated traits: not IEC 559 compliant
    static constexpr bool is_modulo = false;
    static constexpr bool traps = false;
    static constexpr bool tinyness_before = false;
    static constexpr float_round_style round_style = round_toward_zero;
    static constexpr int digits = N; // Approximate, based on bit width
    static constexpr size_t p_min = std::max<size_t>(1, N - 12);
    static constexpr int digits10 = static_cast<int>(p_min * 0.3010);
    static constexpr int max_digits10 = digits10 + 1;
    static constexpr bool is_bounded = true;

    static constexpr double epsilon() noexcept {
        return 2.0 * std::pow(2.0, -static_cast<int>(p_min)); // 2 * ulp(1.0) ≈ machine epsilon
    }
    static constexpr double round_error() noexcept { return 0.5 * epsilon(); }
    static constexpr double min() noexcept { return std::exp(-255.0 * 0.5); } // Approximate min representable
    static constexpr double max() noexcept { return std::exp(255.0 * 0.5); } // Approximate max representable
    static constexpr double lowest() noexcept { return -max(); }
    static constexpr double infinity() noexcept { return std::numeric_limits<double>::infinity(); }
    static constexpr double quiet_NaN() noexcept { return std::numeric_limits<double>::quiet_NaN(); }
    static constexpr double signaling_NaN() noexcept { return quiet_NaN(); }
    static constexpr double denorm_min() noexcept { return min(); } // No subnormals
    static constexpr bool radix = 2;
    static constexpr int max_exponent = 128; // Approximate
    static constexpr int max_exponent10 = 38;
    static constexpr int min_exponent = -127;
    static constexpr int min_exponent10 = -37;
};

} // namespace std