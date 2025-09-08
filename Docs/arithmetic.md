# Arithmetic for Takum (Phase 3)

This document describes the Phase 3 arithmetic implementation added to the project.

## Goals

- Provide correct, immutable arithmetic operators for `takum<N>`.
- Provide safe variants returning `std::expected` (C++26) or `std::optional`.
- Keep implementation simple and correct by using host `double` intermediates for Phase 3.

## Implementation notes

- File: `include/takum/arithmetic.h`
- Operators: `operator+`, `operator-`, `operator*`, `operator/`, `abs`.
- Safe variants: `safe_add`, `safe_mul` returning `std::expected` or `std::optional`.
- NaR handling: Any operation involving NaR or producing non-finite intermediate results yields NaR.

## Future work (Phase 4)

- Replace `operator+` and `operator-` with a Takum-native Gaussian-log (Φ) implementation for improved accuracy and performance.
- Add more comprehensive tests for algebraic identities, associativity (where applicable), and closures.

## Example

See `examples/arithmetic.cpp` for a minimal usage example.
