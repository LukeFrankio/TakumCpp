#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <bitset>
#include <type_traits>
#include <optional>
#include <expected>

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
            uint64_t pat = (N == 64) ? ~uint64_t(0) : ((uint64_t(1) << N) - 1);
            if constexpr (std::is_same_v<storage_t, uint32_t>) t.storage = uint32_t(pat);
            else t.storage = uint64_t(pat);
        } else {
            // for large N just set all words to all-ones (conservative)
            for (size_t i = 0; i < t.storage.size(); ++i) t.storage[i] = ~uint64_t(0);
        }
        return t;
    }

    bool is_nar() const noexcept {
        if constexpr (N <= 64) {
            uint64_t w = uint64_t(storage);
            uint64_t pat = (N == 64) ? ~uint64_t(0) : ((uint64_t(1) << N) - 1);
            return w == pat;
        } else {
            for (auto v : storage) if (v != ~uint64_t(0)) return false;
            return true;
        }
    }

    std::expected<takum, takum_error> to_expected() const noexcept {
        if (is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR"});
        return *this;
    }

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
    static uint64_t encode_from_double(double x) noexcept {
        return ::takum::internal::ref::encode_double_to_bits<N>(x);
    }

    static double decode_to_double(uint64_t bits) noexcept {
        return ::takum::internal::ref::decode_bits_to_double<N>(bits);
    }
};

} // namespace takum
