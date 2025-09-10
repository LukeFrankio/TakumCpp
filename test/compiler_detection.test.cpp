#include <gtest/gtest.h>
#include "takum/compiler_detection.h"
#include <iostream>
#include <string>

/**
 * @file compiler_detection.test.cpp
 * @brief Tests for compiler, platform, and C++ standard detection functionality.
 */

namespace {

TEST(CompilerDetection, CompilerIdentificationMacros) {
    // Ensure exactly one compiler is detected
    int compiler_count = 0;
    if (TAKUM_COMPILER_MSVC) compiler_count++;
    if (TAKUM_COMPILER_GCC) compiler_count++;
    if (TAKUM_COMPILER_CLANG) compiler_count++;
    if (TAKUM_COMPILER_UNKNOWN) compiler_count++;
    
    EXPECT_EQ(compiler_count, 1) << "Exactly one compiler should be detected";
    
    // Verify compiler name is not empty
    EXPECT_GT(std::string(TAKUM_COMPILER_NAME).length(), 0) << "Compiler name should not be empty";
    
    // Verify version numbers are reasonable
    EXPECT_GE(TAKUM_COMPILER_VERSION_MAJOR, 0) << "Compiler major version should be non-negative";
    EXPECT_GE(TAKUM_COMPILER_VERSION_MINOR, 0) << "Compiler minor version should be non-negative";
}

TEST(CompilerDetection, PlatformIdentificationMacros) {
    // Ensure exactly one platform is detected (allow for Unix as umbrella)
    bool windows = TAKUM_PLATFORM_WINDOWS != 0;
    bool linux = TAKUM_PLATFORM_LINUX != 0;
    bool macos = TAKUM_PLATFORM_MACOS != 0;
    bool unix = TAKUM_PLATFORM_UNIX != 0;
    bool unknown = TAKUM_PLATFORM_UNKNOWN != 0;
    
    // At least one platform should be detected
    EXPECT_TRUE(windows || linux || macos || unix || unknown) 
        << "At least one platform should be detected";
    
    // Verify platform name is not empty
    EXPECT_GT(std::string(TAKUM_PLATFORM_NAME).length(), 0) << "Platform name should not be empty";
    
    // Unix should be set for Linux and macOS
    if (linux || macos) {
        EXPECT_TRUE(unix) << "Unix flag should be set for Linux and macOS";
    }
}

TEST(CompilerDetection, CppStandardDetection) {
    // Verify C++ standard version is reasonable
    EXPECT_GE(TAKUM_CPP_VERSION, 17) << "C++ version should be at least 17";
    EXPECT_LE(TAKUM_CPP_VERSION, 30) << "C++ version should not exceed reasonable future values";
    
    // Verify consistency of standard flags
    if (TAKUM_HAS_CPP26) {
        EXPECT_TRUE(TAKUM_HAS_CPP23) << "C++26 should imply C++23";
        EXPECT_TRUE(TAKUM_HAS_CPP20) << "C++26 should imply C++20";
        EXPECT_EQ(TAKUM_CPP_VERSION, 26) << "C++26 flag should match version";
    }
    
    if (TAKUM_HAS_CPP23) {
        EXPECT_TRUE(TAKUM_HAS_CPP20) << "C++23 should imply C++20";
        if (!TAKUM_HAS_CPP26) {
            EXPECT_EQ(TAKUM_CPP_VERSION, 23) << "C++23 flag should match version when C++26 not available";
        }
    }
    
    if (TAKUM_HAS_CPP20 && !TAKUM_HAS_CPP23) {
        EXPECT_EQ(TAKUM_CPP_VERSION, 20) << "C++20 flag should match version when newer standards not available";
    }
}

TEST(CompilerDetection, FeatureDetection) {
    // std::expected should be available in C++23+
    if (TAKUM_HAS_CPP23) {
        EXPECT_TRUE(TAKUM_HAS_STD_EXPECTED) << "std::expected should be available in C++23+";
    }
    
    // std::bit_cast should be available in C++20+
    if (TAKUM_HAS_CPP20) {
        EXPECT_TRUE(TAKUM_HAS_STD_BIT_CAST) << "std::bit_cast should be available in C++20+";
    }
    
    // std::concepts should be available in C++20+
    if (TAKUM_HAS_CPP20) {
        EXPECT_TRUE(TAKUM_HAS_STD_CONCEPTS) << "std::concepts should be available in C++20+";
    }
}

TEST(CompilerDetection, RuntimeInformationFunctions) {
    using namespace takum::compiler_info;
    
    // Test constexpr functions return reasonable values
    EXPECT_GT(std::string(compiler_name()).length(), 0) << "Compiler name should not be empty";
    EXPECT_GT(std::string(platform_name()).length(), 0) << "Platform name should not be empty";
    
    EXPECT_GE(cpp_version(), 17) << "C++ version should be at least 17";
    EXPECT_LE(cpp_version(), 30) << "C++ version should not exceed reasonable future values";
    
    EXPECT_GE(compiler_version_major(), 0) << "Compiler major version should be non-negative";
    EXPECT_GE(compiler_version_minor(), 0) << "Compiler minor version should be non-negative";
    
    // Test feature detection functions
    EXPECT_EQ(has_std_expected(), TAKUM_HAS_STD_EXPECTED != 0);
    EXPECT_EQ(has_std_bit_cast(), TAKUM_HAS_STD_BIT_CAST != 0);
    EXPECT_EQ(has_std_concepts(), TAKUM_HAS_STD_CONCEPTS != 0);
}

TEST(CompilerDetection, CompilerSpecificMacros) {
    // Test that compiler-specific macros are defined
    // We can't test the exact behavior since it's compiler-dependent,
    // but we can verify the macros exist and are properly formed
    
    // Force inline should always be defined
    #ifdef TAKUM_FORCE_INLINE
        SUCCEED() << "TAKUM_FORCE_INLINE is defined";
    #else
        FAIL() << "TAKUM_FORCE_INLINE should be defined";
    #endif
    
    // Restrict should always be defined (may be empty)
    #ifdef TAKUM_RESTRICT
        SUCCEED() << "TAKUM_RESTRICT is defined";
    #endif
    
    // Nodiscard should always be defined
    #ifdef TAKUM_NODISCARD
        SUCCEED() << "TAKUM_NODISCARD is defined";
    #else
        FAIL() << "TAKUM_NODISCARD should be defined";
    #endif
}

TEST(CompilerDetection, PrintDetectedEnvironment) {
    using namespace takum::compiler_info;
    
    // Print detected environment for manual verification in test output
    std::cout << "\n=== Detected Environment ===" << std::endl;
    std::cout << "Compiler: " << compiler_name() 
              << " v" << compiler_version_major() 
              << "." << compiler_version_minor() << std::endl;
    std::cout << "Platform: " << platform_name();
    if (is_64bit_platform()) {
        std::cout << " (64-bit)";
    } else {
        std::cout << " (32-bit)";
    }
    std::cout << std::endl;
    std::cout << "C++ Standard: C++" << cpp_version() << std::endl;
    
    std::cout << "Features:" << std::endl;
    std::cout << "  std::expected: " << (has_std_expected() ? "Yes" : "No") << std::endl;
    std::cout << "  std::bit_cast: " << (has_std_bit_cast() ? "Yes" : "No") << std::endl;
    std::cout << "  std::concepts: " << (has_std_concepts() ? "Yes" : "No") << std::endl;
    std::cout << "===========================" << std::endl;
    
    // This test always passes - it's just for information
    SUCCEED();
}

} // anonymous namespace