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

namespace takum {

// Error type for expected
struct takum_error {
    enum class Kind { DomainError, Overflow, Underflow, InvalidOperation, Inexact, Internal } kind;
    const char* message = nullptr;
};

// Concept mirroring std::floating_point for takum<N> (arithmetic disabled until ready)
template <typename T>
concept takum_floating_point = requires(T t) {
    { t < t } -> std::convertible_to<bool>;
    { t == t } -> std::convertible_to<bool>;
    { t.to_double() } -> std::convertible_to<double>;
    { t.is_nar() } -> std::convertible_to<bool>;
} && sizeof(T) > 0; // Ensure complete type

template <size_t N>
struct takum {
    static_assert(N >= 2 && N <= 256, "takum: supported bit widths 2..256 for now");

    using storage_t = std::conditional_t<(N <= 32), uint32_t,
                      std::conditional_t<(N <= 64), uint64_t, std::array<uint64_t, (N+63)/64>>>;

    storage_t storage{};

    // Default constructs zero
    constexpr takum() noexcept = default;

    // Factory for NaR (canonical pattern for reference impl uses all-ones in N bits stored in lower word)
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

    // Comparison operators following total order: NaR smallest, then negatives to positives monotonic
    bool operator==(const takum& other) const noexcept {
        if (is_nar() && other.is_nar()) return true;
        if (is_nar() || other.is_nar()) return false;
        return storage == other.storage;  // Bitwise equal for reals
    }

    bool operator<(const takum& other) const noexcept {
        if (is_nar() || other.is_nar()) {
            return is_nar() && !other.is_nar();
        }
        // Both real: signed integer compare with sign extension for N bits
        if constexpr (N > 64) {
            // Placeholder for multi-word comparison; for now assume lexicographical signed
            return storage < other.storage;  // TODO: proper multi-word signed compare
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

    // Bitwise inversion on packed storage (Lemma 3: supports inversion-negation patterns)
    takum operator~() const noexcept {
        takum res = *this;
        if constexpr (std::is_integral_v<storage_t>) {
            res.storage = ~res.storage;
        } else {
            for (auto& w : res.storage) {
                w = ~w;
            }
        }
        return res;
    }

    // Unary negation using two's complement on packed storage (Proposition 6: ~x + 1)
    takum operator-() const noexcept {
        if (is_nar()) return *this;
        takum res = *this;
        if constexpr (std::is_integral_v<storage_t>) {
            res.storage = ~res.storage + 1;
        } else {
            // Multi-word two's complement negation: invert all, then add 1 with carry
            for (auto& w : res.storage) {
                w = ~w;
            }
            bool carry = true;
            for (auto& w : res.storage) {
                if (!carry) break;
                uint64_t prev = w;
                w += 1;
                carry = (w < prev);
            }
        }
        return res;
    }

    // Reciprocal (inversion, Proposition 7: mathematical 1/x)
    takum reciprocal() const noexcept {
        if (is_nar() || to_double() == 0.0) return nar();
        return takum(1.0 / to_double());
    }

    // Support for bit_cast and deprecated bit patterns
    storage_t raw_bits() const noexcept { return storage; }

    static takum from_raw_bits(storage_t bits) noexcept {
        takum t{};
        t.storage = bits;
        return t;
    }

#if __cplusplus >= 202302L
    std::expected<takum, takum_error> to_expected() const noexcept {
        if (is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR"});
        return *this;
    }
#else
    std::optional<takum> to_expected() const noexcept {
        if (is_nar()) return std::nullopt;
        return *this;
    }
#endif

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

    // Conversions from host double using the simple reference codec (Phase2 ref)
    explicit takum(double x) noexcept {
        uint64_t bits = takum::encode_from_double(x);
        if constexpr (N <= 64) {
            storage = storage_t(bits);
        } else {
            storage[0] = bits;
            for (size_t i = 1; i < storage.size(); ++i) storage[i] = 0;
        }
    }

    double to_double() const noexcept {
        uint64_t bits;
        if constexpr (N <= 64) {
            bits = uint64_t(storage);
        } else {
            bits = storage[0];
        }
        return takum::decode_to_double(bits);
    }

    // Extract the exact internal logarithmic value: ℓ = (-1)^S * (c + m)
    double get_exact_ell() const noexcept {
        if (N > 128) return 0.0;  // Placeholder for larger widths

        uint64_t bits;
        if constexpr (N <= 64) {
            bits = uint64_t(storage);
        } else {
            bits = 0;  // Placeholder for >64
        }
        if (bits == 0) return 0.0;

        // NaR check
        bool S = (bits >> (N - 1)) & 1ULL;
        uint64_t lower = bits & ((1ULL << (N - 1)) - 1ULL);
        if (S && lower == 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }

        // Extract D and R
        bool D = (bits >> (N - 2)) & 1ULL;
        uint32_t R = (bits >> (N - 5)) & 7ULL;
        uint32_t r = D ? R : (7U - R);

        // Extract C
        uint64_t c_mask = ((1ULL << r) - 1ULL) << (N - 5 - r);
        uint64_t c_bits = (bits & c_mask) >> (N - 5 - r);
        int64_t c = D
            ? (int64_t(((1ULL << r) - 1ULL) + c_bits))
            : (int64_t(-((1LL << (r + 1))) - 1LL + 1LL + c_bits)); // simplifies to -( (1<<(r+1)) - 1 - C )

        // Mantissa
        size_t p = N - 5 - r;
        uint64_t m_bits = bits & ((1ULL << p) - 1ULL);
        double m = (p > 0) ? (static_cast<double>(m_bits) * std::pow(2.0, -static_cast<int>(p))) : 0.0;

        // Return signed ℓ
        double ell_unsigned = static_cast<double>(c) + m;
        return (S ? -1.0 : 1.0) * ell_unsigned;
    }

    // Authoritative maximum representable ell (positive) by decoding the max finite storage
    // Helper to pack the maximum finite positive bit pattern: S=0, D=1, R=max_r, c_bits=max, m=all-ones
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
    static long double max_ell() noexcept {
        if constexpr (N > 64) {
            return 0.0L;  // Placeholder
        }
        uint64_t bits = max_finite_storage();
        takum temp{};
        temp.storage = static_cast<storage_t>(bits);
        return static_cast<long double>(temp.get_exact_ell());
    }

    static uint64_t encode_from_double(double x) noexcept {
        if (x == 0.0) return 0ULL; // Zero representation per eq. (24)
        if (!std::isfinite(x)) return nar().storage;  // NaN/Inf → NaR per NaR convention in Def. 2

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
            uint64_t max_m = (1ULL << p) - 1ULL;
            if (m_bits > max_m) m_bits = max_m; // Clamp
        }

        // Pack bit fields into storage per Def. 2 bit layout: S D R C M
        uint64_t packed = (static_cast<uint64_t>(S) << (N - 1)) | // Sign
                          (static_cast<uint64_t>(D) << (N - 2)) | // Direction
                          (static_cast<uint64_t>(R) << (N - 5)); // Regime
        packed |= (c_bits << p); // Characteristic
        packed |= m_bits; // Mantissa
        return packed;
    }

private:
    // Decode per Def. 2: unpack S,D,R,C,M then compute value per eq. (24)
    static double decode_to_double(uint64_t bits) noexcept {
        if (bits == 0) return 0.0; // Zero per eq. (24)
        // NaR check: S=1 and D=R=C=M=0 per Def. 2
        bool S = (bits >> (N - 1)) & 1ULL; // Sign per eq. (14)
        uint64_t lower = bits & ((1ULL << (N - 1)) - 1ULL);
        if (S && lower == 0) {
            return std::numeric_limits<double>::quiet_NaN(); // NaR per eq. (24)
        }
        // Extract D per eq. (15)
        bool D = (bits >> (N - 2)) & 1ULL;
        // Extract R per eq. (16), compute r per eq. (17)
        uint32_t R = ((bits >> (N - 5)) & 7ULL);
        uint32_t r = D ? R : (7U - R);
        // Extract C per eq. (18), compute c per eq. (19)
        uint64_t c_mask = ((1ULL << r) - 1ULL) << (N - 5 - r);
        uint64_t c_bits = (bits & c_mask) >> (N - 5 - r);
        int64_t c = D
            ? (int64_t(((1ULL << r) - 1ULL) + c_bits)) // D=1 case eq. (19)
            : (int64_t(-((1LL << (r + 1))) + 1LL + c_bits)); // D=0 case eq. (19)
        // p per eq. (20)
        size_t p = N - 5 - r;
        // Extract M per eq. (21), compute m per eq. (22)
        uint64_t m_bits = bits & ((1ULL << p) - 1ULL);
        double m = (p > 0) ? (static_cast<double>(m_bits) * std::pow(2.0, -static_cast<int>(p))) : 0.0;
        // ℓ = c + m per eq. (23)
        double ell = static_cast<double>(c) + m;
        // value = (-1)^S * exp(ℓ / 2) = (-1)^S * √e ^ ℓ per eq. (24)
        double value_sign = S ? -1.0 : 1.0;
        return value_sign * std::exp(ell * 0.5);
    }
};

} // namespace takum

namespace std {

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