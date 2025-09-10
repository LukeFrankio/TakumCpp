// Minimal inline benchmarking utilities for Phi evaluation (Phase-4).
// This header offers a simple timing loop; not compiled unless user includes it
// in a benchmark translation unit. Keeps core library free of <chrono> cost.

#pragma once
#include <cstdint>
#include <chrono>
#include "takum/internal/phi_eval.h"

namespace takum::internal::phi::bench {

template <typename F>
inline uint64_t time_ns(F&& f, size_t iters = 1000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iters; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
}

template <size_t N>
inline long double sweep_sum(size_t samples = 1024) {
    long double acc = 0.0L;
    for (size_t i = 0; i < samples; ++i) {
        long double t = -0.5L + (static_cast<long double>(i) / static_cast<long double>(samples));
        acc += phi_eval<N>(t).value;
    }
    return acc;
}

} // namespace takum::internal::phi::bench