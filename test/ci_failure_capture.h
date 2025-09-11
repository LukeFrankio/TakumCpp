/**
 * @file ci_failure_capture.h
 * @brief Continuous Integration failure logging utilities for TakumCpp test suite.
 *
 * This header provides lightweight logging mechanisms specifically designed for 
 * CI environments to capture and persist test failure information for post-mortem
 * analysis. The utilities are activated only when the CI_CAPTURE_TEST_FAILURES
 * environment variable is set, ensuring zero overhead in development builds.
 *
 * @details
 * **CI Integration Features:**
 * - Environment-controlled activation (CI_CAPTURE_TEST_FAILURES)
 * - Append-only logging to test_failures.log
 * - Structured failure data for automated analysis
 * - Minimal overhead when disabled
 *
 * **Usage Pattern:**
 * ```cpp
 * #include "ci_failure_capture.h"
 * // In test:
 * if (failure_detected) {
 *     ci_capture_failure_line("TestName: specific failure details");
 * }
 * ```
 *
 * @note This is intended for CI/CD pipelines and automated testing environments
 * @note The log file is created in the current working directory
 * @see test_helpers.h for complementary test utilities
 */

#pragma once
#include <fstream>
#include <cstdlib>
#include <string>

/**
 * @brief Capture test failure information to CI log file.
 *
 * Writes a single line of failure information to the CI capture log file
 * (test_failures.log) when the CI_CAPTURE_TEST_FAILURES environment variable
 * is set. This function is designed for use in test cases to provide
 * detailed failure information for automated analysis.
 *
 * @param line Complete failure description string to log
 * 
 * @details
 * **Behavior:**
 * - Does nothing if CI_CAPTURE_TEST_FAILURES environment variable is unset
 * - Creates or appends to test_failures.log in the current directory
 * - Each call writes exactly one line (newline automatically added)
 * - Silently ignores file creation/write errors to avoid test disruption
 *
 * **Log Format:**
 * The function accepts any string format but structured JSON is recommended:
 * ```
 * {"test":"TestName","detail":"specific failure info","bits":"0x1234"}
 * ```
 *
 * @note This function has minimal overhead and can be called from hot test paths
 * @note File operations are synchronous; suitable for single-threaded test execution
 */
inline void ci_capture_failure_line(const std::string &line) {
    const char* env = std::getenv("CI_CAPTURE_TEST_FAILURES");
    if (!env) return;
    std::ofstream f("test_failures.log", std::ios::app);
    if (!f) return;
    f << line << std::endl;
}
