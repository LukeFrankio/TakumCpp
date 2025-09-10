# TakumCpp GitHub Copilot Instructions

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

TakumCpp is a C++ header-only library implementing logarithmic tapered floating-point (Takum) arithmetic. The library is template-based with `takum<N>` for N ≥ 12 bits, uses immutable values, and provides NaR (Not-a-Real) handling for error states.

## Working Effectively

### Bootstrap, Build, and Test the Repository
Run these commands in sequence to get a working development environment:

- `mkdir build` -- Create build directory
- `python3 scripts/gen_poly_coeffs.py` -- Generate coefficient header (takes 3 seconds)
- `cmake -S . -B build` -- Configure build system (takes 5 seconds). Set timeout to 30+ seconds.
- `cmake --build build` -- **NEVER CANCEL: Build takes 50 seconds. Set timeout to 5+ minutes.**
- `ctest --test-dir build --output-on-failure` -- **NEVER CANCEL: Test suite takes 6 seconds, 63 tests total. Set timeout to 1+ minute.**

### Alternative Quick Build Commands
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` -- Configure for Release build
- `cmake --build build --config Release` -- **NEVER CANCEL: Release build takes 47 seconds. Set timeout to 5+ minutes.**

### Validate Examples Work
Always run these after any changes to core functionality:
- `cd build && ./arithmetic` -- Should output: `x=1.5 y=2.25 z=3.875`
- `cd build && ./basic_usage` -- Should show takum operations and bit patterns

## Validation

### ALWAYS run through at least one complete end-to-end scenario after making changes:
1. Run `python3 scripts/gen_poly_coeffs.py` -- Verify coefficient generation works
2. Run `cmake --build build` -- Verify your changes compile
3. Run `ctest --test-dir build --output-on-failure` -- Verify all 63 tests pass
4. Run example programs in `build/` directory -- Verify examples execute correctly
5. Run `ctest --test-dir build --output-on-failure --verbose` if any test failures need investigation

### Manual Validation Requirements
- **CRITICAL**: After making any arithmetic changes, run both example programs and verify output matches expected values
- After adding new tests, run the full test suite to ensure no regressions
- After modifying headers in `include/takum/`, rebuild completely to check for compilation errors
- For multi-precision changes (N > 64), specifically test takum128 operations

## Build System Details

### Dependencies and Requirements
- **C++ Standard**: C++23 (automatically detected, falls back from C++26 if unavailable)
- **CMake**: 3.20+ required
- **Python3**: Required for coefficient generation (scripts/gen_poly_coeffs.py)
- **GoogleTest**: Automatically downloaded during build via FetchContent

### Build Timing Expectations
- **NEVER CANCEL: Full build takes 50 seconds on typical systems**
- CMake configuration: 5 seconds
- Python coefficient generation: 3 seconds  
- Test execution: 6 seconds (63 tests)
- Example execution: <1 second each

### Build Targets
- `TakumCpp` -- Header-only interface library target
- `tests` -- All unit tests (builds automatically)
- `arithmetic` -- Example program
- `basic_usage` -- Example program  
- `docs` -- Doxygen documentation (requires doxygen in PATH)
- `phi_coeffs_gen` -- Generates polynomial coefficients header

### Platform Notes
- **Linux/Unix**: Use standard make/ninja generators
- **Windows**: Use `build.bat` or `build.ps1` (wraps cmake with MinGW/MSYS2 detection)
- **CI**: Uses Windows with MSYS2/MinGW via GitHub Actions (.github/workflows/ci.yml)

## Code Structure and Key Files

### Essential Header Files (include/takum/)
- `core.h` -- Main takum<N> template, encoding/decoding, NaR handling
- `arithmetic.h` -- Basic arithmetic operations (+, -, *, /, safe variants)
- `types.h` -- Common type aliases (takum32, takum64, takum128)
- `compatibility.h` -- Deprecated shims with compile warnings
- `internal/phi_eval.h` -- Gaussian-log (Φ) evaluation for addition
- `internal/generated/phi_coeffs.h` -- Generated polynomial coefficients

### Test Organization (test/)
- All tests follow pattern: `*.test.cpp` files auto-discovered by CMake
- Add new tests as `SomeName.test.cpp` using GoogleTest framework
- Test categories: arithmetic, core, multiword, compatibility, phi, etc.
- Use existing test patterns from `arithmetic.test.cpp` for new functionality

### Generated Files
- `include/takum/internal/generated/phi_coeffs.h` -- Created by `scripts/gen_poly_coeffs.py`
- **DO NOT** manually edit generated files
- Regenerate coefficients when modifying Φ (Gaussian-log) logic

## Common Tasks

### Adding New Arithmetic Operations
1. Add function declaration to `include/takum/arithmetic.h`
2. Add implementation (inline for templates)
3. Add tests to appropriate `*.test.cpp` file
4. Verify with: `cmake --build build && ctest --test-dir build --output-on-failure`

### Adding New Tests
1. Create `YourFeature.test.cpp` in `test/` directory
2. Use GoogleTest macros: `TEST(TestSuite, TestName) { ... }`
3. Include required headers: `#include "takum/core.h"` etc.
4. Build and run: **NEVER CANCEL: Build + test cycle takes 61 seconds total**

### Modifying Core Encoding/Decoding
1. Edit `include/takum/core.h` -- Maintain NaR patterns and monotonic ordering  
2. Run full test suite to verify no regressions
3. Test multi-word sizes (takum128) explicitly
4. Verify examples still produce expected output

## Debugging and Validation Commands

### When Tests Fail
- `ctest --test-dir build --output-on-failure --verbose` -- Detailed test output
- `cd build && ./test/tests --gtest_filter="FailingTest.*"` -- Run specific test
- Check `build/test_results.log` for captured test output

### When Build Fails  
- Verify Python3 available: `python3 --version`
- Regenerate coefficients: `python3 scripts/gen_poly_coeffs.py`
- Clean rebuild: `rm -rf build && mkdir build && cmake -S . -B build && cmake --build build`

### Performance and Timing Validation
- Debug build: **50 seconds** (add 50% buffer = 75 second timeout)
- Release build: **47 seconds** (add 50% buffer = 70 second timeout)  
- Test suite: **6 seconds** (recommend 15+ minute timeout = 900+ seconds, very large buffer)

## Critical Constraints and Standards

### DO NOT
- Introduce stateful globals or singletons -- Keep pure functional design
- Assume IEEE 754 behaviors -- Use `is_nar()` not `std::isnan()`
- Use in-place mutating operations -- takum<N> values are immutable
- Bypass existing encode/decode helpers when constructing from host values
- Cancel long-running builds or tests -- Full validation is essential

### ALWAYS
- Follow 2-space indentation, ≤100 character lines (see `Docs/CodingStandards.md`)
- Use `snake_case` for functions, `PascalCase` for types
- Add Doxygen `///` comments for public APIs
- Maintain NaR propagation semantics (any NaR input → NaR output)
- Test both single-word (N ≤ 64) and multi-word (N > 64) takum sizes
- Run the complete build and test cycle before submitting changes

### Error Handling Philosophy
- Return NaR for invalid operations (division by zero, overflow)
- Use `std::expected` (C++26) or `std::optional` fallback for safe variants
- Never throw exceptions -- Use NaR or safe variant return types
- Maintain total ordering: NaR < any real number, NaR == NaR

## Performance and Configuration Hooks

### Conditional Compilation Flags
- `TAKUM_ENABLE_FAST_ADD` -- Experimental Φ-based addition optimization
- `TAKUM_ENABLE_CUBIC_PHI_LUT` -- Catmull-Rom cubic interpolation for small LUTs
- `TAKUM_COARSE_LUT_SIZE` -- Hybrid coarse LUT size for takum64+ (default: 256)
- `TAKUM_ENABLE_PHI_DIAGNOSTICS` -- Lightweight performance counters

### When Modifying Φ (Gaussian-log) Infrastructure
- Regenerate coefficients: `python3 scripts/gen_poly_coeffs.py`
- Use dispatch via `takum::internal::phi::phi_eval<N>(t)` 
- Do not hard-code polynomial details
- Test LUT consistency and clamping behavior

## Validation Scenarios

The following represent critical user workflows that must continue working:

### Basic Arithmetic Workflow
```cpp
#include "takum/core.h"
#include "takum/arithmetic.h"

takum<32> x(3.14);
takum<32> y(2.0);
auto sum = x + y;
auto product = x * y;
// Should compile and run without errors
```

### Safe Arithmetic Workflow  
```cpp
auto safe_result = safe_add(x, y);
if (safe_result.has_value()) {
    // Use safe_result.value()
}
// Should handle NaR and overflow cases gracefully
```

### Multi-precision Workflow
```cpp
takum<128> big_x(1e100);
takum<128> big_y(1e-100);  
auto big_sum = big_x + big_y;
// Should handle multi-word arithmetic correctly
```

## Quick Reference: Common Outputs

### Directory Structure
```
.
├── .github/                 # GitHub workflows and configurations  
├── CMakeLists.txt          # Root build configuration
├── Docs/                   # Documentation and coding standards
├── build/                  # Build output directory (create with mkdir)
├── debug/                  # Debug programs (empty in current repo)
├── examples/               # Example programs (arithmetic.cpp, basic_usage.cpp)
├── include/takum/          # Header-only library source
├── scripts/                # Build and generation scripts
└── test/                   # Unit tests (*.test.cpp files)
```

### Expected Example Output
- `./arithmetic` → `x=1.5 y=2.25 z=3.875`
- `./basic_usage` → Shows operations, comparisons, NaR handling, bit patterns

### Core Domain Knowledge
- **Central type**: `template<size_t N> takum<N>` (N ≥ 12 bits)
- **Storage**: N≤32 → uint32_t, N≤64 → uint64_t, N>64 → array<uint64_t>
- **NaR (Not-a-Real)**: Sign bit set, all other bits zero, always propagates
- **Immutability**: No in-place operations, pure functional design
- **Encoding**: (S D R C M) fields per specification, uses host double intermediates
- **Φ infrastructure**: Gaussian-log approximations for optimized addition/subtraction