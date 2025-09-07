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

struct takum_error {
    enum class Kind { DomainError, Overflow, Underflow, InvalidOperation, Inexact, Internal } kind;
    const char* message = nullptr;
};

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
            uint64_t unsigned_val = uint64_t(storage);
            int64_t signed_val = static_cast<int64_t>( (unsigned_val << (64 - N)) >> (64 - N) );
            uint64_t other_unsigned_val = uint64_t(other.storage);
            int64_t other_signed_val = static_cast<int64_t>( (other_unsigned_val << (64 - N)) >> (64 - N) );
            return signed_val < other_signed_val;
        }
    }

    bool operator<=(const takum& other) const noexcept { return !(other < *this); }
    bool operator>(const takum& other) const noexcept { return other < *this; }
    bool operator>=(const takum& other) const noexcept { return !(*this < other); }
    bool operator!=(const takum& other) const noexcept { return !(*this == other); }

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


static uint64_t encode_from_double(double x) noexcept {
    if (x == 0.0) return 0ULL;
    if (!std::isfinite(x)) return nar().storage;  // NaN/Inf → NaR

    bool S = std::signbit(x);
    long double abs_x = std::fabsl(x);

    long double ell = 2.0L * std::logl(abs_x);

    // Clamp
    long double clamp_pos = max_ell();
    if (ell > clamp_pos) ell = clamp_pos;
    if (ell < -clamp_pos) ell = -clamp_pos;

    // Decompose into integer and fractional
    int64_t c = static_cast<int64_t>(std::floorl(ell));
    bool D = (c >= 0);
    int64_t abs_c = D ? c : -c;

    // Regime
    uint32_t r = (abs_c != 0)
        ? static_cast<uint32_t>(std::floorl(std::log2l(D ? abs_c + 1 : abs_c)))
        : 0;
    r = std::min<uint32_t>(7, r);
    uint32_t R = D ? r : (7U - r);

    // C bits
    uint64_t c_bits = 0ULL;
    if (r != 0) {
        if (D) {
            c_bits = static_cast<uint64_t>(c - ((1ULL << r) - 1ULL));
        } else {
            c_bits = static_cast<uint64_t>(c + ((1ULL << (r+1)) - 1ULL));
        }
    }

    // Fractional
    long double m = ell - static_cast<long double>(c);
    if (m < 0.0L) m = 0.0L;
    if (m >= 1.0L) m = 0.999999L;  // avoid overflow

    size_t p = N - 5 - static_cast<size_t>(r);
    uint64_t m_bits = 0ULL;
    if (p > 0 && m > 0.0L) {
        long double m_power = std::ldexpl(1.0L, static_cast<int>(p));
        long double m_scaled_ld = m * m_power;
        m_bits = static_cast<uint64_t>(std::floorl(m_scaled_ld + 0.5L));
        uint64_t max_m = (1ULL << p) - 1ULL;
        if (m_bits > max_m) m_bits = max_m;
    }

    // Pack
    uint64_t packed = (static_cast<uint64_t>(S) << (N - 1)) |
                      (static_cast<uint64_t>(D) << (N - 2)) |
                      (static_cast<uint64_t>(R) << (N - 5));
    packed |= (c_bits << p);
    packed |= m_bits;
    return packed;
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

  private:
    // Accurate decode: unpack bits to fields, compute ℓ, then value = sign * exp(ℓ / 2)
    static double decode_to_double(uint64_t bits) noexcept {
        if (bits == 0) return 0.0;
        // Check NaR: S=1 and all other bits=0
        bool S = (bits >> (N - 1)) & 1ULL;
        uint64_t lower = bits & ((1ULL << (N - 1)) - 1ULL);
        if (S && lower == 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Extract D: bit N-2
        bool D = (bits >> (N - 2)) & 1ULL;
        // Extract R: bits N-5 to N-3 (R2=N-5, R1=N-4, R0=N-3)
        uint32_t R = ((bits >> (N - 5)) & 7ULL);
        uint32_t r = D ? R : (7U - R);
        // Extract C: next r bits, starting from bit N-6 down to N-5-r
        uint64_t c_mask = ((1ULL << r) - 1ULL) << (N - 5 - r);
        uint64_t c_bits = (bits & c_mask) >> (N - 5 - r);
        int64_t c = D ? (int64_t(((1ULL << r) - 1ULL) + c_bits)) : (int64_t(-((1LL << (r + 1))) + 1LL + c_bits));
        // p = N - 5 - r
        size_t p = N - 5 - r;
        // M: lowest p bits
        uint64_t m_bits = bits & ((1ULL << p) - 1ULL);
        double m = (p > 0) ? (static_cast<double>(m_bits) * std::pow(2.0, -static_cast<int>(p))) : 0.0;
        // ℓ = c + m
        double ell = static_cast<double>(c) + m;
        // value = (-1)^S * exp(ℓ / 2)
        double value_sign = S ? -1.0 : 1.0;
        return value_sign * std::exp(ell * 0.5);
    }

};

} // namespace takum
