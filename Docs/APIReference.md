# TakumCpp API Reference

This document provides comprehensive API documentation for the TakumCpp library, focusing on the user-facing interface and configuration options.

## Core Types

### `takum<N>`

The main numeric type template representing an N-bit takum value.

```cpp
template<size_t N>
class takum {
    static_assert(N >= 12, "Takum requires at least 12 bits");
    // ...
};
```

**Template Parameters:**
- `N`: Bit width (must be ≥ 12 for valid takum encoding)

**Type Aliases:**
```cpp
using takum16 = takum<16>;   // Half-precision equivalent
using takum32 = takum<32>;   // Single-precision equivalent  
using takum64 = takum<64>;   // Double-precision equivalent
using takum128 = takum<128>; // Quad-precision equivalent
```

### Construction and Conversion

#### Constructors
```cpp
constexpr takum() noexcept;              // Zero value
explicit takum(double value) noexcept;   // From double
explicit constexpr takum(storage_type bits) noexcept; // From bits (internal)
```

#### Static Factory Methods
```cpp
static constexpr takum nar() noexcept;   // Create NaR value
static constexpr takum zero() noexcept;  // Create zero value
static takum one() noexcept;             // Create unity value
```

#### Conversion Methods
```cpp
double to_double() const noexcept;            // Convert to double
long double get_exact_ell() const noexcept;   // Get ℓ-space value
constexpr storage_type bits() const noexcept; // Get bit representation
```

### Value Testing

```cpp
constexpr bool is_nar() const noexcept;    // Check if NaR
constexpr bool is_zero() const noexcept;   // Check if zero
constexpr bool is_finite() const noexcept; // Check if finite (not NaR)
```

### Arithmetic Operations

#### Basic Arithmetic
```cpp
takum operator+(const takum& other) const noexcept; // Addition
takum operator-(const takum& other) const noexcept; // Subtraction  
takum operator*(const takum& other) const noexcept; // Multiplication
takum operator/(const takum& other) const noexcept; // Division
takum operator-() const noexcept;                   // Unary negation
takum abs() const noexcept;                         // Absolute value
```

#### Safe Arithmetic (with error detection)
```cpp
expected<takum> safe_add(const takum& other) const noexcept;
expected<takum> safe_subtract(const takum& other) const noexcept;
expected<takum> safe_multiply(const takum& other) const noexcept;
expected<takum> safe_divide(const takum& other) const noexcept;
```

**Return Type**: `expected<T>` is an alias for `std::optional<T>` providing:
- `has_value()`: Check if operation succeeded
- `value()`: Get result (throws if no value)
- `value_or(default)`: Get result or default

### Comparison Operations

```cpp
constexpr bool operator==(const takum& other) const noexcept;
constexpr bool operator!=(const takum& other) const noexcept;
constexpr bool operator<(const takum& other) const noexcept;
constexpr bool operator<=(const takum& other) const noexcept;
constexpr bool operator>(const takum& other) const noexcept;
constexpr bool operator>=(const takum& other) const noexcept;
```

**Ordering**: Follows total ordering with NaR as smallest value:
- NaR < any finite value
- NaR == NaR is true
- Finite values follow natural mathematical ordering

## Configuration System

### Runtime Configuration

The new configuration system replaces compile-time macros with runtime options:

```cpp
#include "takum/config/runtime_config.h"
using namespace takum::config;
```

#### Core Configuration Functions
```cpp
// Get configuration manager instance
configuration_manager& manager = configuration_manager::instance();

// Set configuration values
manager.set<bool>("enable_cubic_interpolation", true);
manager.set<size_t>("coarse_lut_size", 512);
manager.set<std::string>("phi_strategy", "hybrid");

// Get configuration values  
bool cubic_enabled = manager.get<bool>("enable_cubic_interpolation");
size_t lut_size = manager.get<size_t>("coarse_lut_size");
```

#### Convenience Functions
```cpp
namespace takum::config::options {
    // Φ strategy selection
    std::string phi_strategy();
    void set_phi_strategy(const std::string& strategy);
    
    // LUT configuration
    size_t coarse_lut_size();
    void set_coarse_lut_size(size_t size);
    
    // Feature toggles
    bool enable_cubic_interpolation();
    void set_enable_cubic_interpolation(bool enable);
    
    bool enable_phi_diagnostics();
    void set_enable_phi_diagnostics(bool enable);
    
    bool enable_fast_add();
    void set_enable_fast_add(bool enable);
}
```

### Available Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `phi_strategy` | string | "auto" | Φ evaluation strategy: "polynomial", "lut", "hybrid", "auto" |
| `coarse_lut_size` | size_t | 256 | Size of coarse LUT for hybrid strategy |
| `enable_cubic_interpolation` | bool | false | Use cubic instead of linear interpolation |
| `enable_phi_diagnostics` | bool | false | Enable performance monitoring |
| `enable_fast_add` | bool | false | Enable fast addition heuristics |

### Configuration Scopes

Temporary configuration changes using RAII:

```cpp
#include "takum/config/runtime_config.h"

{
    // Temporarily change strategy
    config::config_scope scope("phi_strategy", "polynomial");
    
    // All operations in this scope use polynomial strategy
    takum32 result = a + b;
    
} // Configuration automatically restored
```

## Arithmetic Engine Strategies

### Strategy Information

```cpp
// Get current strategy information
std::string info = takum32::get_arithmetic_strategy_info();
// Returns: "hybrid (accuracy: 1.5e-12)"

// Configure strategy for a precision
takum64::configure_arithmetic_strategy("polynomial");
```

### Available Strategies

#### Polynomial Strategy
- **Best for**: High precision (takum64+)
- **Accuracy**: Highest
- **Performance**: Moderate
- **Memory**: Low

#### LUT Strategy  
- **Best for**: Low precision (takum16, takum32)
- **Accuracy**: Good
- **Performance**: Highest
- **Memory**: Moderate to High

#### Hybrid Strategy
- **Best for**: Balanced requirements (takum32+)
- **Accuracy**: High  
- **Performance**: Good
- **Memory**: Moderate

## Stream I/O

```cpp
#include <iostream>
#include "takum/takum.h"

takum32 value(3.14159);

// Output
std::cout << value << std::endl;  // Prints: 3.14159

// Input
takum32 input;
std::cin >> input;  // Reads double value and converts

// Special values
std::cout << takum32::nar() << std::endl;  // Prints: NaR
```

## String Conversion

```cpp
#include "takum/takum.h"

// To string
takum32 value(2.718);
std::string str = to_string(value);  // "2.718"

// From string
takum32 parsed = from_string<32>("3.14159");
takum32 nar_val = from_string<32>("NaR");
takum32 invalid = from_string<32>("invalid");  // Returns NaR
```

## Error Handling

### Error Types

```cpp
enum class takum_error_kind {
    domain_error,      // Input outside valid domain
    overflow,          // Result too large
    underflow,         // Result too small  
    invalid_operation, // Undefined operation
    inexact,          // Cannot represent exactly
    internal_error    // Internal computation error
};

struct takum_error {
    takum_error_kind kind;
    std::string message;
};
```

### Safe Operations Usage

```cpp
takum32 a(1e30);
takum32 b(1e30);

auto result = a.safe_add(b);
if (result.has_value()) {
    std::cout << "Result: " << result.value() << std::endl;
} else {
    std::cout << "Addition failed (overflow)" << std::endl;
}

// With default fallback
takum32 safe_result = a.safe_add(b).value_or(takum32::nar());
```

## Thread Safety

- **Immutable values**: All takum objects are immutable and thread-safe
- **Configuration**: Configuration changes are thread-safe with internal locking
- **Arithmetic engines**: Thread-local caching ensures thread safety
- **Global state**: Minimal global state, properly synchronized

## Performance Considerations

### Memory Layout
- **Packed storage**: Optimal memory usage (uint32_t, uint64_t, or array)
- **Cache friendly**: Structures designed for cache efficiency
- **SIMD ready**: Bit patterns optimized for vectorization

### Computational Efficiency
- **Exact operations**: Multiplication and division are exact in ℓ-space
- **Optimized Φ**: Strategy selection optimizes speed/accuracy tradeoffs
- **Minimal branches**: NaR handling designed to minimize conditional code

### Best Practices
1. **Choose appropriate precision**: Use smallest sufficient bit width
2. **Configure strategies**: Select optimal Φ strategy for your workload
3. **Batch operations**: Group similar operations for better cache usage
4. **Profile first**: Use diagnostics to identify performance bottlenecks

## Migration from Previous Versions

### Macro Migration
Old compile-time macros are replaced with runtime configuration:

```cpp
// Old approach (compile-time)
#define TAKUM_ENABLE_FAST_ADD 1
#define TAKUM_COARSE_LUT_SIZE 512

// New approach (runtime)
config::options::set_enable_fast_add(true);
config::options::set_coarse_lut_size(512);
```

### Header Changes
```cpp
// Old headers
#include "takum/core.h"
#include "takum/arithmetic.h"

// New unified header
#include "takum/takum.h"
```

The new API maintains backward compatibility while providing enhanced functionality and better performance characteristics.