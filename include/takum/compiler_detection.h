#pragma once

/**
 * @file compiler_detection.h
 * @brief Comprehensive compiler, platform, and C++ standard detection for TakumCpp.
 *
 * This header provides macros and utilities for detecting the compiler, platform,
 * and C++ standard features to enable proper compatibility across:
 * - Compilers: MSVC, GCC, Clang
 * - Platforms: Windows, Linux, macOS
 * - Standards: C++20, C++23, C++26
 *
 * The detection macros allow conditional compilation and compiler-specific
 * workarounds throughout the TakumCpp library.
 */

// =============================================================================
// Compiler Detection
// =============================================================================

#undef TAKUM_COMPILER_MSVC
#undef TAKUM_COMPILER_GCC
#undef TAKUM_COMPILER_CLANG
#undef TAKUM_COMPILER_UNKNOWN

#if defined(_MSC_VER)
    #define TAKUM_COMPILER_MSVC 1
    #define TAKUM_COMPILER_VERSION_MAJOR (_MSC_VER / 100)
    #define TAKUM_COMPILER_VERSION_MINOR ((_MSC_VER % 100) / 10)
    #define TAKUM_COMPILER_NAME "MSVC"
#elif defined(__clang__)
    // AppleClang has different capabilities than regular Clang
    #if defined(__apple_build_version__)
        #define TAKUM_COMPILER_APPLE_CLANG 1
        #define TAKUM_COMPILER_CLANG 0
        #define TAKUM_COMPILER_NAME "AppleClang"
    #else
        #define TAKUM_COMPILER_CLANG 1
        #define TAKUM_COMPILER_APPLE_CLANG 0
        #define TAKUM_COMPILER_NAME "Clang"
    #endif
    #define TAKUM_COMPILER_VERSION_MAJOR __clang_major__
    #define TAKUM_COMPILER_VERSION_MINOR __clang_minor__
#elif defined(__GNUC__)
    #define TAKUM_COMPILER_GCC 1
    #define TAKUM_COMPILER_VERSION_MAJOR __GNUC__
    #define TAKUM_COMPILER_VERSION_MINOR __GNUC_MINOR__
    #define TAKUM_COMPILER_NAME "GCC"
#else
    #define TAKUM_COMPILER_UNKNOWN 1
    #define TAKUM_COMPILER_VERSION_MAJOR 0
    #define TAKUM_COMPILER_VERSION_MINOR 0
    #define TAKUM_COMPILER_NAME "Unknown"
#endif

// Ensure one compiler is detected
#ifndef TAKUM_COMPILER_MSVC
    #define TAKUM_COMPILER_MSVC 0
#endif
#ifndef TAKUM_COMPILER_GCC
    #define TAKUM_COMPILER_GCC 0
#endif
#ifndef TAKUM_COMPILER_CLANG
    #define TAKUM_COMPILER_CLANG 0
#endif
#ifndef TAKUM_COMPILER_APPLE_CLANG
    #define TAKUM_COMPILER_APPLE_CLANG 0
#endif
#ifndef TAKUM_COMPILER_UNKNOWN
    #define TAKUM_COMPILER_UNKNOWN 0
#endif

// =============================================================================
// Platform Detection
// =============================================================================

#undef TAKUM_PLATFORM_WINDOWS
#undef TAKUM_PLATFORM_LINUX
#undef TAKUM_PLATFORM_MACOS
#undef TAKUM_PLATFORM_UNIX
#undef TAKUM_PLATFORM_UNKNOWN

#if defined(_WIN32) || defined(_WIN64)
    #define TAKUM_PLATFORM_WINDOWS 1
    #define TAKUM_PLATFORM_NAME "Windows"
    #ifdef _WIN64
        #define TAKUM_PLATFORM_64BIT 1
    #else
        #define TAKUM_PLATFORM_64BIT 0
    #endif
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define TAKUM_PLATFORM_MACOS 1
        #define TAKUM_PLATFORM_NAME "macOS"
    #endif
    #define TAKUM_PLATFORM_UNIX 1
    #define TAKUM_PLATFORM_64BIT 1  // Modern macOS is always 64-bit
#elif defined(__linux__)
    #define TAKUM_PLATFORM_LINUX 1
    #define TAKUM_PLATFORM_UNIX 1
    #define TAKUM_PLATFORM_NAME "Linux"
    #if defined(__x86_64__) || defined(__aarch64__)
        #define TAKUM_PLATFORM_64BIT 1
    #else
        #define TAKUM_PLATFORM_64BIT 0
    #endif
#elif defined(__unix__) || defined(__unix)
    #define TAKUM_PLATFORM_UNIX 1
    #define TAKUM_PLATFORM_NAME "Unix"
    #if defined(__x86_64__) || defined(__aarch64__)
        #define TAKUM_PLATFORM_64BIT 1
    #else
        #define TAKUM_PLATFORM_64BIT 0
    #endif
#else
    #define TAKUM_PLATFORM_UNKNOWN 1
    #define TAKUM_PLATFORM_NAME "Unknown"
    #define TAKUM_PLATFORM_64BIT 0
#endif

// Ensure all platform macros are defined
#ifndef TAKUM_PLATFORM_WINDOWS
    #define TAKUM_PLATFORM_WINDOWS 0
#endif
#ifndef TAKUM_PLATFORM_LINUX
    #define TAKUM_PLATFORM_LINUX 0
#endif
#ifndef TAKUM_PLATFORM_MACOS
    #define TAKUM_PLATFORM_MACOS 0
#endif
#ifndef TAKUM_PLATFORM_UNIX
    #define TAKUM_PLATFORM_UNIX 0
#endif
#ifndef TAKUM_PLATFORM_UNKNOWN
    #define TAKUM_PLATFORM_UNKNOWN 0
#endif

// =============================================================================
// C++ Standard Detection
// =============================================================================

#undef TAKUM_CPP_VERSION
#undef TAKUM_HAS_CPP20
#undef TAKUM_HAS_CPP23
#undef TAKUM_HAS_CPP26

// Detect C++ standard version
#if __cplusplus >= 202600L
    #define TAKUM_CPP_VERSION 26
    #define TAKUM_HAS_CPP26 1
    #define TAKUM_HAS_CPP23 1
    #define TAKUM_HAS_CPP20 1
#elif __cplusplus >= 202100L // Partial C++23 implementations (e.g., GCC 13 with -std=c++23)
    #define TAKUM_CPP_VERSION 23
    #define TAKUM_HAS_CPP26 0
    #define TAKUM_HAS_CPP23 1
    #define TAKUM_HAS_CPP20 1
#elif __cplusplus >= 202002L
    #define TAKUM_CPP_VERSION 20
    #define TAKUM_HAS_CPP26 0
    #define TAKUM_HAS_CPP23 0
    #define TAKUM_HAS_CPP20 1
#else
    #define TAKUM_CPP_VERSION 17
    #define TAKUM_HAS_CPP26 0
    #define TAKUM_HAS_CPP23 0
    #define TAKUM_HAS_CPP20 0
#endif

// =============================================================================
// Feature Detection
// =============================================================================

// Check for header availability first
#ifdef __has_include
    #define TAKUM_HAS_INCLUDE(header) __has_include(header)
#else
    #define TAKUM_HAS_INCLUDE(header) 0
#endif

// Conservative std::expected detection
// Primary reliance on feature test macro, with compiler version fallbacks for known working combinations
#if TAKUM_HAS_CPP23 && TAKUM_HAS_INCLUDE(<expected>) && !defined(TAKUM_NO_STD_EXPECTED)
    // Prefer the feature test macro when available (most reliable)
    #if defined(__cpp_lib_expected)
        #define TAKUM_HAS_STD_EXPECTED 1
    // Fallback to conservative compiler + standard library combinations that are known to work
    #elif (defined(_MSC_VER) && _MSC_VER >= 1930) || \
          (defined(__GNUC__) && __GNUC__ >= 14 && defined(__GLIBCXX__))
        #define TAKUM_HAS_STD_EXPECTED 1
    #else
        #define TAKUM_HAS_STD_EXPECTED 0
    #endif
#else
    #define TAKUM_HAS_STD_EXPECTED 0
#endif

// std::bit_cast availability (C++20 with header check)
#if TAKUM_HAS_CPP20 && TAKUM_HAS_INCLUDE(<bit>) && !defined(TAKUM_NO_STD_BIT_CAST)
    #define TAKUM_HAS_STD_BIT_CAST 1
#else
    #define TAKUM_HAS_STD_BIT_CAST 0
#endif

// std::concepts availability (C++20 with header check)
#if TAKUM_HAS_CPP20 && TAKUM_HAS_INCLUDE(<concepts>) && !defined(TAKUM_NO_STD_CONCEPTS)
    #define TAKUM_HAS_STD_CONCEPTS 1
#else
    #define TAKUM_HAS_STD_CONCEPTS 0
#endif

// Constexpr std::bit_width (C++20 with bit header)
#if TAKUM_HAS_STD_BIT_CAST && !defined(TAKUM_NO_CONSTEXPR_BIT_WIDTH)
    #define TAKUM_HAS_CONSTEXPR_BIT_WIDTH 1
#else
    #define TAKUM_HAS_CONSTEXPR_BIT_WIDTH 0
#endif

// Additional compiler-specific feature detection
#if TAKUM_COMPILER_MSVC
    // MSVC-specific feature detection
    #if TAKUM_COMPILER_VERSION_MAJOR >= 193  // VS 2022 17.3+
        #define TAKUM_MSVC_HAS_CPP23_FEATURES 1
    #else
        #define TAKUM_MSVC_HAS_CPP23_FEATURES 0
    #endif
#elif TAKUM_COMPILER_GCC
    // GCC-specific feature detection  
    #if TAKUM_COMPILER_VERSION_MAJOR >= 12
        #define TAKUM_GCC_HAS_CPP23_FEATURES 1
    #else
        #define TAKUM_GCC_HAS_CPP23_FEATURES 0
    #endif
#elif TAKUM_COMPILER_CLANG
    // Clang-specific feature detection
    #if TAKUM_COMPILER_VERSION_MAJOR >= 15
        #define TAKUM_CLANG_HAS_CPP23_FEATURES 1
    #else
        #define TAKUM_CLANG_HAS_CPP23_FEATURES 0
    #endif
#endif

// =============================================================================
// Compiler-Specific Configurations
// =============================================================================

// Warning control
#if TAKUM_COMPILER_MSVC
    #define TAKUM_PRAGMA_WARNING_PUSH __pragma(warning(push))
    #define TAKUM_PRAGMA_WARNING_POP __pragma(warning(pop))
    #define TAKUM_PRAGMA_WARNING_DISABLE_MSVC(x) __pragma(warning(disable: x))
    #define TAKUM_PRAGMA_WARNING_DISABLE_GCC(x)
    #define TAKUM_PRAGMA_WARNING_DISABLE_CLANG(x)
#elif TAKUM_COMPILER_GCC
    #define TAKUM_PRAGMA_WARNING_PUSH _Pragma("GCC diagnostic push")
    #define TAKUM_PRAGMA_WARNING_POP _Pragma("GCC diagnostic pop")
    #define TAKUM_PRAGMA_WARNING_DISABLE_MSVC(x)
    #define TAKUM_PRAGMA_WARNING_DISABLE_GCC(x) _Pragma("GCC diagnostic ignored " #x)
    #define TAKUM_PRAGMA_WARNING_DISABLE_CLANG(x)
#elif TAKUM_COMPILER_CLANG
    #define TAKUM_PRAGMA_WARNING_PUSH _Pragma("clang diagnostic push")
    #define TAKUM_PRAGMA_WARNING_POP _Pragma("clang diagnostic pop")
    #define TAKUM_PRAGMA_WARNING_DISABLE_MSVC(x)
    #define TAKUM_PRAGMA_WARNING_DISABLE_GCC(x)
    #define TAKUM_PRAGMA_WARNING_DISABLE_CLANG(x) _Pragma("clang diagnostic ignored " #x)
#else
    #define TAKUM_PRAGMA_WARNING_PUSH
    #define TAKUM_PRAGMA_WARNING_POP
    #define TAKUM_PRAGMA_WARNING_DISABLE_MSVC(x)
    #define TAKUM_PRAGMA_WARNING_DISABLE_GCC(x)
    #define TAKUM_PRAGMA_WARNING_DISABLE_CLANG(x)
#endif

// Force inline
#if TAKUM_COMPILER_MSVC
    #define TAKUM_FORCE_INLINE __forceinline
#elif TAKUM_COMPILER_GCC || TAKUM_COMPILER_CLANG
    #define TAKUM_FORCE_INLINE inline __attribute__((always_inline))
#else
    #define TAKUM_FORCE_INLINE inline
#endif

// Restrict keyword
#if TAKUM_COMPILER_MSVC
    #define TAKUM_RESTRICT __restrict
#elif TAKUM_COMPILER_GCC || TAKUM_COMPILER_CLANG
    #define TAKUM_RESTRICT __restrict__
#else
    #define TAKUM_RESTRICT
#endif

// Likely/unlikely attributes (C++20)
#if TAKUM_HAS_CPP20
    #define TAKUM_LIKELY [[likely]]
    #define TAKUM_UNLIKELY [[unlikely]]
#else
    #define TAKUM_LIKELY
    #define TAKUM_UNLIKELY
#endif

// No discard attribute
#if __cplusplus >= 201703L
    #define TAKUM_NODISCARD [[nodiscard]]
#else
    #define TAKUM_NODISCARD
#endif

// =============================================================================
// Compiler-Specific Workarounds
// =============================================================================

// MSVC-specific configurations
#if TAKUM_COMPILER_MSVC
    // Disable specific MSVC warnings that are overly pedantic for header-only libraries
    #pragma warning(push)
    #pragma warning(disable: 4127)  // conditional expression is constant
    #pragma warning(disable: 4505)  // unreferenced local function has been removed
#endif

// GCC-specific configurations
#if TAKUM_COMPILER_GCC
    // GCC 9+ has better constexpr support
    #if TAKUM_COMPILER_VERSION_MAJOR >= 9
        #define TAKUM_GCC_CONSTEXPR_FRIENDLY 1
    #else
        #define TAKUM_GCC_CONSTEXPR_FRIENDLY 0
    #endif
#endif

// Clang-specific configurations
#if TAKUM_COMPILER_CLANG
    // Clang has excellent C++20/23 support from version 12+
    #if TAKUM_COMPILER_VERSION_MAJOR >= 12
        #define TAKUM_CLANG_MODERN 1
    #else
        #define TAKUM_CLANG_MODERN 0
    #endif
#endif

// =============================================================================
// Runtime Environment Information
// =============================================================================

namespace takum {
namespace compiler_info {

/**
 * @brief Get compiler name as a compile-time string.
 * @return String literal containing the compiler name.
 */
constexpr const char* compiler_name() noexcept {
    return TAKUM_COMPILER_NAME;
}

/**
 * @brief Get platform name as a compile-time string.
 * @return String literal containing the platform name.
 */
constexpr const char* platform_name() noexcept {
    return TAKUM_PLATFORM_NAME;
}

/**
 * @brief Get detected C++ standard version.
 * @return Integer representing the C++ standard (20, 23, 26, etc.).
 */
constexpr int cpp_version() noexcept {
    return TAKUM_CPP_VERSION;
}

/**
 * @brief Get compiler version major number.
 * @return Major version number of the detected compiler.
 */
constexpr int compiler_version_major() noexcept {
    return TAKUM_COMPILER_VERSION_MAJOR;
}

/**
 * @brief Get compiler version minor number.
 * @return Minor version number of the detected compiler.
 */
constexpr int compiler_version_minor() noexcept {
    return TAKUM_COMPILER_VERSION_MINOR;
}

/**
 * @brief Check if running on 64-bit platform.
 * @return true if platform is 64-bit, false otherwise.
 */
constexpr bool is_64bit_platform() noexcept {
    return TAKUM_PLATFORM_64BIT != 0;
}

/**
 * @brief Check if std::expected is available.
 * @return true if std::expected can be used, false otherwise.
 */
constexpr bool has_std_expected() noexcept {
    return TAKUM_HAS_STD_EXPECTED != 0;
}

/**
 * @brief Check if std::bit_cast is available.
 * @return true if std::bit_cast can be used, false otherwise.
 */
constexpr bool has_std_bit_cast() noexcept {
    return TAKUM_HAS_STD_BIT_CAST != 0;
}

/**
 * @brief Check if std::concepts is available.
 * @return true if std::concepts can be used, false otherwise.
 */
constexpr bool has_std_concepts() noexcept {
    return TAKUM_HAS_STD_CONCEPTS != 0;
}

} // namespace compiler_info
} // namespace takum

// Restore MSVC warnings
#if TAKUM_COMPILER_MSVC
    #pragma warning(pop)
#endif