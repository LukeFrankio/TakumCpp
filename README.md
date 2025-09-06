# TakumCpp: A Logarithmic Tapered Floating-Point Arithmetic Library

TakumCpp is a C++ library implementing the Takum arithmetic format as described in the research paper. It provides a high-performance, logarithmic tapered floating-point system with properties like monotonicity, total-ordering, and NaR (Not a Real) handling. The library prefers C++26 and leverages `std::expected` for error propagation, packed integer storage for efficiency, and full coverage of C++ floating-point features including deprecations; guarded fallbacks keep the project usable on C++23 toolchains.

## Features
- Template-based precision: `takum<N>` for N ≥ 12 bits.
- Immutable, pure functions following functional programming principles.
- Gaussian-log approximations for addition/subtraction using LUT + interpolation or hybrid polynomials.
- Compatibility shims for deprecated C++ features with compile-time warnings.
- Integration with standard library components like `<cmath>`, `<complex>`, `<random>`, etc.
- `<stdfloat>` interop where available.

## Requirements
- C++26 capable compiler recommended (GCC/Clang recent, MSVC 2022+); guarded fallbacks support C++23 toolchains.
- CMake 3.20+.

## Build Instructions
1. Clone the repository:
   ```
   git clone https://github.com/yourusername/TakumCpp.git
   cd TakumCpp
   ```

2. Create build directory and configure:
   ```
   mkdir build
   cd build
   cmake ..
   ```

3. Build the project:
   ```
   cmake --build .
   ```

4. Run tests (optional):
   ```
   ctest
   ```

For development builds with sanitizers, use:
```
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## Usage
Include the headers:
```cpp
#include <takum/core.h>
#include <takum/arithmetic.h>
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