#pragma once

#include "core.h"
#include "types.h"

// Deprecated fixed-width shims (e.g., non-standard float8 -> takum<8>)
using float8_t = takum::takum<8>;

#pragma message("Deprecated: float8_t is a shim for non-standard float8 using takum<8> with ghost bits. Prefer takum<8>.")

#if __cplusplus < 202302L
// Shim for std::expected if C++26 not available
#include <optional>

namespace takum {
template <typename T, typename E>
struct expected_shim {
    std::optional<T> value;
    E error;

    bool has_value() const { return value.has_value(); }
    T value_or(const T& default_val) const { return value.value_or(default_val); }
    const T& operator*() const { return *value; }
};

} // namespace takum
#endif