# Debug Programs

This directory contains standalone debugging and testing programs for TakumCpp development.

## Building Debug Programs

To build all debug programs:

```bash
cd debug
mkdir build
cmake -S . -B build
cmake --build build
```

To build a specific debug program:

```bash
cd debug
mkdir build
cmake -S . -B build
cmake --build build --target debug_simple
```

## Running Debug Programs

After building, executables will be in the `debug/build/` directory:

```bash
cd debug/build
./debug_simple
./debug_add
./debug_phi
# etc.
```

## Available Debug Programs

- `debug_simple` - Simple takum arithmetic test
- `debug_add` - Addition operation testing
- `debug_sub` - Subtraction operation testing
- `debug_phi` - Phi function testing
- `debug_128` - 128-bit takum testing
- `debug_basic` - Basic takum operations
- `debug_from_ell` - From ell construction testing
- `debug_full_test` - Full feature testing
- And more...

## Note

These programs are for development and debugging purposes only. The main library tests are in the `test/` directory.