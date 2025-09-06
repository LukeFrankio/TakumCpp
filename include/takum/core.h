#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <bitset>
#include <type_traits>
#include <optional>
#if __cplusplus >= 202302L
#include <expected>
#else
#include <variant>  // Fallback shim if needed, but use optional for now
#endif

#include "takum/internal/ref/tau_ref.h"

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
            uint64_t pat = 1ULL << (N - 1);  // Only sign bit set
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
        if constexpr (N <= 64) {
            uint64_t bits = takum::encode_from_double(x);
            storage = storage_t(bits);
        } else {
            // naive: store lower 64 bits from reference
            uint64_t bits = takum::encode_from_double(x);
            storage[0] = bits;
            for (size_t i = 1; i < storage.size(); ++i) storage[i] = 0;
        }
    }

    double to_double() const noexcept {
        if constexpr (N <= 64) {
            uint64_t bits = uint64_t(storage);
            return takum::decode_to_double(bits);
        } else {
            uint64_t bits = storage[0];
            return takum::decode_to_double(bits);
        }
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
        // ℓ = pow(-1, S) * (c + m)
        double ell = (S ? -1.0 : 1.0) * (static_cast<double>(c) + m);
        // value = pow(-1, S) * exp(ell / 2)
        double value_sign = S ? -1.0 : 1.0;
        return value_sign * std::exp(ell * 0.5);
    }

    // Accurate encode per Algorithm 2: double -> fields -> pack bits
    static uint64_t encode_from_double(double x) noexcept {
        if (x == 0.0) return 0ULL;
        if (!std::isfinite(x)) return nar().storage;  // NaN/inf -> NaR

        bool S = std::signbit(x);
        double y = std::fabs(x);
        double ell = 2.0 * std::log(y);
        // Saturate ell to (-255, 255)
        if (ell > 254.999) ell = 254.999;
        if (ell < -254.999) ell = -254.999;
        double signed_ell = S ? -ell : ell;
        int64_t c = static_cast<int64_t>(std::floor(signed_ell));
        bool D = (c >= 0);
        uint32_t r;
        if (D) {
            r = static_cast<uint32_t>(std::floor(std::log2(static_cast<double>(c + 1))));
        } else {
            r = static_cast<uint32_t>(std::floor(std::log2(static_cast<double>(-c))));
        }
        if (r > 7) r = 7;  // cap regime
        uint32_t R = D ? r : (7U - r);
        uint64_t c_bits;
        if (r == 0) {
            c_bits = 0ULL;
        } else {
            if (D) {
                c_bits = static_cast<uint64_t>(c - ((1ULL << r) - 1ULL));
            } else {
                c_bits = static_cast<uint64_t>(c + ((1ULL << (r + 1)) - 1ULL));
            }
        }
        double m = signed_ell - static_cast<double>(c);
        if (m < 0.0) m = 0.0;
        if (m >= 1.0) m = 0.999999;  // avoid overflow
        size_t p = N - 5 - r;
        uint64_t m_bits = 0ULL;
        if (p > 0 && m > 0.0) {
            m_bits = static_cast<uint64_t>(std::round(m * static_cast<double>(1ULL << p)));
            if (m_bits >= (1ULL << p)) m_bits = (1ULL << p) - 1ULL;
        }
        // Pack: MSB S (bit N-1), D (N-2), R (N-5 to N-3), C_bits (next r bits), m_bits (lowest p bits)
        uint64_t packed = (static_cast<uint64_t>(S) << (N - 1)) |
                          (static_cast<uint64_t>(D) << (N - 2)) |
                          (static_cast<uint64_t>(R) << (N - 5));
        packed |= (c_bits << p);
        packed |= m_bits;
        return packed;
    }
};

} // namespace takum
