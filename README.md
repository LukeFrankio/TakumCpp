# TakumCpp: Advanced Logarithmic Tapered Floating-Point Arithmetic Library

TakumCpp is a modern C++ library implementing the Takum arithmetic format - a revolutionary logarithmic tapered floating-point system that provides superior precision characteristics compared to IEEE 754 arithmetic. This library offers production-ready performance with comprehensive mathematical foundations, extensive documentation, and practical examples demonstrating real-world advantages.

## 🚀 Key Features

### Mathematical Excellence
- **Logarithmic Number System**: Unique ℓ-space (logarithmic space) representation providing better precision distribution
- **Total Ordering**: All values including NaR (Not-a-Real) maintain consistent ordering relationships  
- **Monotonic Encoding**: Bit patterns preserve numerical ordering for efficient comparisons
- **Wide Dynamic Range**: Superior representation across many orders of magnitude

### Modern C++ Design
- **Template-Based Precision**: `takum<N>` for any N ≥ 12 bits (takum16, takum32, takum64, takum128)
- **Immutable Value Semantics**: Pure functional design with no global state
- **C++23/C++26 Compatible**: Modern language features with fallback support
- **Header-Only Library**: Easy integration with zero build dependencies

### Production Quality
- **Comprehensive Testing**: 69+ test cases covering edge cases, precision, and performance
- **Performance Optimized**: Gaussian-log approximations with LUT interpolation
- **Memory Efficient**: Packed storage optimized for cache performance  
- **Cross-Platform**: Works on Linux, Windows, macOS with major compilers

## 📊 Real-World Applications

TakumCpp excels in scenarios where precision and dynamic range matter:

- **Financial Computing**: Micro-transactions, compound interest, percentage calculations
- **Scientific Computing**: Wide dynamic range simulations, iterative algorithms
- **High-Precision Analytics**: Accumulative calculations with reduced error propagation
- **Embedded Systems**: Efficient arithmetic in resource-constrained environments

## 🎯 Quick Start

### Installation

```bash
git clone https://github.com/LukeFrankio/TakumCpp.git
cd TakumCpp
mkdir build
python3 scripts/gen_poly_coeffs.py  # Generate polynomial coefficients
cmake -S . -B build                # Configure build
cmake --build build                # Build (takes ~50s)
ctest --test-dir build             # Run tests (6s, 69 tests)
```

### Basic Usage

```cpp
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

int main() {
    // Create takum values
    takum32 a{3.14159};
    takum32 b{2.71828};
    
    // Arithmetic operations
    takum32 sum = a + b;
    takum32 product = a * b;
    
    // High-precision output
    std::cout << "Sum: " << sum.to_double() << std::endl;
    std::cout << "Product: " << product.to_double() << std::endl;
    
    // NaR handling
    takum32 invalid = takum32::nar();
    std::cout << "Is NaR: " << invalid.is_nar() << std::endl;
    
    return 0;
}
```

## 📈 Performance Demonstrations

The library includes comprehensive examples showing takum's advantages:

### Financial Precision Example
```bash
./build/financial_precision
```
Demonstrates superior precision in:
- Compound interest calculations
- Micro-transaction accumulation  
- Small percentage representations
- Portfolio calculations

### Scientific Computing Example
```bash
./build/scientific_computing
```
Shows advantages in:
- Wide dynamic range representation
- Iterative algorithms (Newton-Raphson)
- Monte Carlo simulations
- Exponential decay modeling

### Performance Benchmark
```bash
./build/performance_benchmark
```
Comprehensive performance analysis covering:
- Basic arithmetic operations
- Vector operations  
- Conversion overhead
- Memory usage characteristics

## 🏗️ Architecture Overview

### Core Components

- **`takum<N>`**: Main numeric type with configurable precision
- **Arithmetic Engine**: Optimized operations using Gaussian-log approximations
- **Storage System**: Multi-word support for arbitrary precision (N > 64)
- **Type System**: Complete integration with C++ numeric facilities

### Mathematical Foundation

TakumCpp implements the complete mathematical theory of takum numbers:

- **ℓ-space Representation**: `ℓ = log₂|x|` for logarithmic encoding
- **Regime-Exponent-Mantissa**: Structured bit layout for optimal precision
- **Gaussian-log Functions**: Φ(t) functions for addition/subtraction
- **NaR Semantics**: Total ordering with error value handling

### Precision Levels

| Type | Bits | Approximate Range | Best Use Case |
|------|------|------------------|---------------|
| `takum16` | 16 | ±10³⁸ | Embedded systems, graphics |
| `takum32` | 32 | ±10¹⁵⁴ | General computing, financial |
| `takum64` | 64 | ±10⁹⁹³² | Scientific computing, ML |
| `takum128` | 128 | ±10⁴⁰⁰⁰⁰⁺ | Ultra-high precision research |

## 📚 Documentation

### Complete Documentation Suite

- **[API Reference](Docs/APIReference.md)**: Complete function and class documentation
- **[Mathematical Foundation](Docs/MathematicalFoundation.md)**: Theoretical background and proofs
- **[Performance Guide](Docs/Performance.md)**: Optimization tips and benchmarks  
- **[Migration Guide](Docs/Migration.md)**: Porting from IEEE 754 arithmetic

### Example Programs

| Example | Description | Features Demonstrated |
|---------|-------------|-----------------------|
| `basic_usage` | Simple operations | Construction, conversion, comparisons |
| `arithmetic` | Basic arithmetic | Addition, multiplication, output |
| `financial_precision` | Financial computing | Compound interest, micro-transactions |
| `scientific_computing` | Scientific applications | Monte Carlo, iterative algorithms |
| `performance_benchmark` | Performance analysis | Speed comparisons, memory usage |

## 🧪 Testing and Validation

### Comprehensive Test Suite

- **69 Test Cases**: Covering all major functionality
- **Edge Case Testing**: NaR handling, overflow, underflow
- **Precision Validation**: Round-trip accuracy, monotonicity
- **Performance Testing**: Benchmarks against IEEE 754
- **Multi-Precision Testing**: All supported bit widths

### Build Validation

```bash
# Run full test suite
ctest --test-dir build --output-on-failure

# Run with verbose output
ctest --test-dir build --verbose

# Run examples as integration tests
cd build
./arithmetic        # Should output: x=1.5 y=2.25 z=3.875
./basic_usage       # Should show operations and bit patterns
```

## ⚙️ Configuration and Tuning

### Build Options

```bash
# Release build for production
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Debug build with sanitizers
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Enable all warnings as errors
cmake -S . -B build -DTAKUM_WARNINGS_AS_ERRORS=ON

# Build with documentation
cmake -S . -B build -DTAKUM_BUILD_DOCS=ON
```

### Performance Tuning

The library includes several optimization opportunities:

- **LUT Size Configuration**: Adjust lookup table sizes for speed/memory trade-offs
- **Polynomial Strategies**: Choose between speed and precision for Φ evaluation
- **Multi-threading**: Examples show vectorization opportunities
- **Memory Layout**: Cache-friendly data structures for large arrays

## 🔬 Research and Development

### Contributing

TakumCpp welcomes contributions in:

- **Algorithm Improvements**: Better Φ approximations, faster arithmetic
- **Platform Support**: Additional architectures, compiler support
- **Application Examples**: New use cases demonstrating takum advantages
- **Performance Optimization**: SIMD, GPU acceleration, vectorization

### Future Directions

- **SIMD Vectorization**: Parallel operations for array processing
- **GPU Acceleration**: CUDA/OpenCL implementations
- **Extended Precision**: Support for takum256, takum512, arbitrary precision
- **Language Bindings**: Python, Julia, Rust interfaces

## 📄 License and Citation

TakumCpp is released under the MIT License. See LICENSE file for details.

If you use TakumCpp in research, please cite:
```bibtex
@software{takumcpp2024,
  title={TakumCpp: A C++ Implementation of Takum Arithmetic},
  author={[Authors]},
  year={2024},
  url={https://github.com/LukeFrankio/TakumCpp}
}
```

## 🤝 Community and Support

- **GitHub Issues**: Bug reports and feature requests
- **Discussions**: General questions and usage help
- **Documentation**: Comprehensive guides and examples
- **Examples**: Real-world applications and benchmarks

---

**TakumCpp**: Advancing numerical computing through innovative logarithmic arithmetic.
```

Example:
```cpp
#include <takum/core.h>
#include <iostream>

int main() {
    takum<32> x = takum<32>::encode(3.14);
    takum<32> y = takum<32>::encode(2.0);
    auto sum = x + y;
    std::cout << takum<32>::decode(sum) << std::endl;
    return 0;
}
```

## Documentation
See `docs/` for specifications, coding standards, and API details.

## License
MIT License (see LICENSE).