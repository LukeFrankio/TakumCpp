// Minimal helper: when running under CI (CI_CAPTURE_TEST_FAILURES env var),
// tests can include this header and write reproducible failure logs to
// test_failures.log for later inspection.
#pragma once
#include <fstream>
#include <cstdlib>

inline void ci_capture_failure_line(const std::string &line) {
    const char* env = std::getenv("CI_CAPTURE_TEST_FAILURES");
    if (!env) return;
    std::ofstream f("test_failures.log", std::ios::app);
    if (!f) return;
    f << line << std::endl;
}
