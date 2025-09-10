### TakumCpp Modular Architecture Implementation Plan

Following comprehensive refactoring analysis, this plan implements a modern modular architecture for the TakumCpp library that addresses the identified structural issues and establishes a foundation for long-term scalability.

**Architectural Principles:**
- Separation of concerns: Core encoding, arithmetic engines, and configuration systems
- Runtime configurability replacing compile-time macros
- Modular Φ evaluation strategies with pluggable implementations
- Thread-safe operations with immutable value semantics
- Comprehensive error handling and type safety

**Implementation Status:**
✅ Core encoding/decoding separation (`include/takum/core/encoding.h`)
✅ Modular arithmetic engine with strategy pattern (`include/takum/arithmetic/arithmetic_engine.h`)
✅ Runtime configuration system (`include/takum/config/runtime_config.h`)
✅ Refactored main takum type (`include/takum/takum.h`)
🔄 Mathematical documentation restructuring (in progress)
🔄 Build system modernization (in progress)
🔄 Testing infrastructure enhancement (in progress)

#### Phase 1: Research and Specification (2-4 weeks, Low Effort)
  - **Goals**: Deepen understanding of Takum from the paper and map to C++ floating-point features (from prior responses: types, operators, `<cmath>`, etc.). Define library scope, FP constraints, and edge cases (e.g., NaR propagation per Gustafson criteria). Ensure 100% coverage by auditing all features, including deprecations (e.g., `<math.h>` aliases, C++26 reverted deprecations like certain `<cmath>` quiet NaN behaviors). Incorporate constraints: C++26 min, packed storage rationale, Φ approaches (LUT for <64-bit, hybrid for >32-bit), `std::expected` for NaR, compatibility shims with warnings.
   - **Steps**:
     1. Review Takum encoding/decoding (Definition 2 in paper): Implement pseudocode for `tau_inv` (encoding) and `tau` (decoding) using fixed-point logs for ℓ (logarithmic value). Specify packed storage (uint32_t etc.) and debug bitset view. **Warning: Conceptual Confusion - ℓ vs τ**: Tests/PRs must compare full τ (including sign S) for uniqueness/injectivity (per paper's proof); comparing ℓ alone is incorrect as different (S, c+m) can yield equal ℓ but distinct τ. Always use full canonical form or to_double() in tests.
     2. Analyze LNS alternatives: Study logarithmic converters [ieeexplore.ieee.org](https://ieeexplore.ieee.org/document/8268821/) for Gaussian log approximations (Φ_b^± for addition/subtraction) and [ieeexplore.ieee.org](https://ieeexplore.ieee.org/document/9458421) for low-precision LNS in edge devices. Detail LUT params (1024/4096 sizes, fixed-point interp) and hybrid poly (degree-5..7 minimax, pre-gen coeffs).
     3. Map C++ features: List all floating-point usages (e.g., `std::sin(double)` → `takum_sin(takum)`). Include deprecations: e.g., `<math.h>` funcs (sinf → shim with #pragma message), obsolete subnormals in proprietary formats (bfloat16/TF32 no subnormals → Takum saturation).
     4. Specify variants: Logarithmic (default, per paper) and linear Takum (Definition 8) as templates. Define NaR: `std::expected<takum<N>, takum_error>` with `is_nar()` helper.
     5. Define precision templates: `template<size_t N> struct takum;` (N ≥ 12 bits; packed storage policy: uint32_t for N<=32, uint64_t for <=64, array<uint64_t,K> for larger).
     6. Handle special cases: 0, NaR (per Definition 7: total-ordering), saturation (Algorithm 1). Add deprecation handling: Shims with #pragma warnings in compatibility.h.
    7. Exhaustive audit: Chronological list by standard (C++98 to C++26), covering deprecations (e.g., C++11 deprecated some old rounding, C++26 reverts `<cmath>` NaN pow(NaN,0)=1). Include <stdfloat> interop (e.g., float16_t → takum16).
    - **Dependencies**: C++26 standard, paper PDF.
   - **Milestones**: Requirements doc (Markdown/PDF) with UML diagrams for types/ops; pseudocode for core encoding; full 100% coverage matrix (table of features → replacements); LUT/poly specs.
   - **Deliverables**: `spec.md` (Takum traits like `std::numeric_limits` analogs); initial CMakeLists.txt.
   - **Files Created**:
     - `docs/spec.md`: Full requirements doc with UML, pseudocode for encoding/decoding, FP constraints, exhaustive coverage matrix (all features/deprecations), and mapping to C++ features; include storage/Φ/NaR/compat details.
  - `CMakeLists.txt`: Initial root CMake setup (enable C++26, add subdirs; target_compile_features PUBLIC cxx_std_26).
     - `README.md`: High-level overview, build instructions.
     - `test/placeholder.test.cpp`: Empty test skeleton for future phases.

#### Phase 2: Core Type and Encoding Implementation (4-6 weeks, Medium Effort)
   - **Goals**: Implement immutable `takum<N>` struct with encoding/decoding, supporting FP immutability. Cover deprecated type behaviors (e.g., implementation-defined `long double` → portable Takum<128>). Use packed storage (uint32_t/etc.); debug bitset view. Integrate <stdfloat> interop (e.g., convert std::float16_t → takum16).
   - **Steps**:
     1. Define struct: `template<size_t N> struct takum { /* packed storage: using storage_t = uint32_t; */ storage_t bits; };` (uint32_t for N<=32, uint64_t <=64, array<uint64_t,K> >64; optional debug `std::bitset<N> debug_view() const;`).
     2. Implement decoding (Algorithm 3): Pure function `takum decode(double x);` using `std::log`, `std::exp` (base √e via `log(x) / log(sqrt(M_E))`); constexpr where possible.
     3. Implement encoding (Algorithm 2): `storage_t encode(double x);` with saturation (Algorithm 1); use `std::floor(std::log2)` for regime r; <stdfloat> overloads (e.g., encode(std::float32_t)).
     4. Add constructors: From `double`/`float` (lossy), literals (user-defined, C++11+ e.g., `1.0f16` → `takum16{1.0};` for deprecated hex floats); from <stdfloat> types. **Test Comparisons**: To avoid ties or ordering flips from converting takum to double (which can collapse distinct values due to rounding), use high-precision long double for decoded internal tuple (S, c, m) or ℓ comparisons (option A) for round-trip/monotonicity; alternatively, compare to_double() allowing <= or detect ties and fallback to canonical encodings (option B). Prioritize option A via ref decoder to fully guard against rounding issues.
     5. FP traits: Concept `takum_floating_point` (C++20) mirroring `std::floating_point`; specialize `std::numeric_limits<takum<N>>` (epsilon as λ(p) from Proposition 11). Cover deprecated traits (e.g., old `std::is_floating_point` for `long double` variants).
     6. Bitwise ops from paper: `operator~` for inversion-negation (Lemma 3, on packed storage), `~takum + 1` for negation (Proposition 6), inversion (Proposition 7). Add bit_cast for deprecated bit patterns (use storage_t).
     7. Handle short bitstrings (<12): Ghost bits (zero-padding in packed). Cover deprecated fixed-width (e.g., non-standard float8 → takum<8> shim via compatibility.h).
   - **Dependencies**: Phase 1 spec.
   - **Milestones**: Compile-time tests for encoding/decoding accuracy (e.g., round-trip using high-precision long double for decoded internal tuple (S, c, m) or ℓ within λ(p) to avoid rounding ties/collapses from double conversions); validate deprecated `long double` conversions and <stdfloat> interop. **Note on Ordering**: For monotonicity tests (Prop 4), reorder iteration from unsigned 0..2^n-1 to SI order (starting at 1<<(n-1), wrapping to nar_index+1 .. 2^n-1, 0 .. nar_index-1) to match paper's ascending τ order; unsigned order will falsely fail monotonicity. Since NaR is special (canonical pattern, smallest value per total-ordering), skip neighbor comparisons across NaR or special-case it (e.g., check monotonicity separately for real values, excluding NaR boundary); use high-precision long double for decoded comparisons (option A) or tie-detection with canonical encodings (option B) to prevent false order flips from precision loss.
   - **Deliverables**: Header `takum/core.h`; tests for uniqueness (Proposition 3), monotonicity (Proposition 4).
   - **Files Created**:
     - `include/takum/core.h`: Core `template<size_t N> struct takum;` with packed storage (uint32_t/etc.), constructors, encoding/decoding funcs (`tau`, `tau_inv`), traits (`takum_floating_point` concept), `std::numeric_limits` specialization, literals, bit_cast overloads, debug bitset view, <stdfloat> interop (e.g., takum<32>(std::float32_t)), deprecated type shims (e.g., `long_double_shim`).
     - `include/takum/types/takum_precision.h`: Precision aliases (e.g., `using takum16 = takum<16>;`, `using takum32 = takum<32>;` for half/float-like; one shared header for all common precisions: 8 (deprecated float8), 16,19 (TF32),32,64,128,256; include shims for proprietary like bfloat16/TF32 with packed storage).
     - `test/core.test.cpp`: Unit tests for decoding/encoding (Prop 8), uniqueness (Prop 3), monotonicity (Prop 4), special cases (0, NaR), deprecated conversions (e.g., float8 ghost bits), <stdfloat> round-trip.
     - `examples/basic_usage.cpp`: Simple example: `takum32 x = encode(3.14); auto y = decode(x);` including deprecated literal like `0x1.2p3_tk` and std::float32_t interop.
     - `docs/core.md`: Doxygen-integrated docs for core type, including packed storage, <stdfloat> notes, deprecated behaviors.

#### Phase 3: Basic Arithmetic Operators (4 weeks, Medium Effort)
   - **Goals**: Implement pure, immutable operators as FP functions, replacing `+`, `-`, `*`, `/`, etc. Cover deprecated arithmetic (e.g., old % for floats in C++98, though rarely used). Use packed storage for bitwise ops; `std::expected` for NaR in safe variants.
   - **Steps**:
     1. Unary: `takum operator-() const;` (bitwise negation +1 on packed, Proposition 6); `takum abs() const;` (sign flip). Shim deprecated unary + for consistency.
     2. Binary: Multiplication/division as ℓ addition/subtraction (Section 4.3: ℓ_x + ℓ_y on packed); pure `takum add(takum a, takum b);` using Gaussian logs Φ_√e^±(q) placeholder (detailed Phase 4); safe: `std::expected<takum, takum_error> safe_add(takum a, takum b);` with `is_nar(result)`. Cover deprecated mixed-type (e.g., int + float → takum promote).
     3. Power ops: Square (`ℓ * 2`), sqrt (`ℓ / 2`), inversion (bitwise on packed, Proposition 7). Deprecated pow(0,0)=1 shim with warning.
     4. Comparisons: `<`, `==` via two's complement order (Proposition 4 on packed); NaR as smallest (Definition 7). Cover deprecated NaN != NaN (use total-ordering shim with warning).
     5. FP wrappers: Higher-order `auto plus = [](auto&& f) { return [f](takum a, takum b) { return f(a, b); }; };` for composition; `to_expected()` helper.
     6. Rounding: `round_n(takum x, size_t n);` (Algorithm 1, geometric mean in log domain). Cover deprecated rounding modes (e.g., old toward-zero) with shims.
     7. Overflow/Underflow: Saturate to bounds (√e^{±255}). Shim deprecated infinities (map to NaR with warning).
   - **Dependencies**: Phase 2 core type.
   - **Milestones**: 100% exactness for mul/div/sqrt (Section 5.6 closure analysis); test deprecated mixed ops and `std::expected` NaR.
   - **Deliverables**: `takum/arithmetic.h`; FP pipeline example: `std::ranges::fold_left(v, takum{1}, std::plus<>{});`.
   - **Files Created**:
     - `include/takum/arithmetic.h`: Pure operators (`operator+`, `-`, `*`, `/`, `%` shim, unary `-`/`+`, `abs`), bitwise negation/inversion (Props 6-7 on packed), comparisons; FP functors (e.g., `struct plus_t { takum operator()(takum a, takum b) const; };`); `std::expected` safe variants, `is_nar()`/`to_expected()` helpers; deprecated shims (e.g., old NaN comparisons with #pragma message).
     - `test/arithmetic.test.cpp`: Tests for ops exactness (mul/div closure), comparisons (NaR total-ordering), rounding (Algorithm 1), deprecated behaviors (e.g., pow(NaN,0)=NaR shim), `std::expected` paths.
     - `examples/arithmetic.cpp`: FP pipeline: `std::ranges::accumulate(v.begin(), v.end(), takum32{1}, plus_t{});` including deprecated mixed-type and safe_add example.
     - `docs/arithmetic.md`: Docs with FP composition examples, deprecation warnings, packed storage notes.

#### Phase 4: Mathematical Functions (6-8 weeks, High Effort)
   - **Goals**: FP-style overloads for `<cmath>` equivalents, using Takum for args/results. Full coverage of all `<cmath>` funcs, including deprecated (e.g., `<math.h>` C-style like `sinf`, `logl`; C++26 reverted deprecations like `pow(1, NaN)=1` → NaR prop). Implement Gaussian-log (Φ) as specified: LUT+interp for takum16/takum32 (1024/4096 entries, fixed-point linear/cubic); hybrid for takum64+ (256-entry coarse LUT + minimax poly degree-5..7, pre-gen coeffs). Use packed storage; `std::expected` for NaR-prone funcs.
   - **Steps**:
     1. Trig/Hyperbolic: `takum sin(takum x);` via series in log domain or table lookup (adapt from LNS [dl.acm.org](https://dl.acm.org/doi/10.1145/3461699)); use Φ for add/sub in compound funcs. Overload all variants (sinf/sind/sinl shims with warnings). LUT for low N, poly for high.
     2. Exp/Log: `takum exp(takum x);` (direct from ℓ: √e^ℓ); `takum log(takum x);` (ℓ extraction). Cover deprecated log(0)= -inf → saturation with `std::expected`.
     3. Power/Root: `takum pow(takum base, takum exp);` (ℓ_base * ℓ_exp). Shim deprecated pow(NaN,0)=1 with NaR and warning.
     4. Rounding/Classify: `takum floor(takum x);`, `isnan(takum x);` (NaR check via helper). All modes (floorf/floorl); deprecated fpclassify with shim.
     5. Advanced: `hypot`, `cbrt` (reduce to log ops with Φ hybrid); lerp (linear interp in linear space, but pure func). Cover deprecated hypot variants; use minimax for high N.
     6. FP: Monadic `std::expected<takum, takum_error> safe_sin(takum x);` for NaR propagation; optional fallback.
     7. Constants: `<numbers>`-like `std::numbers::pi_v<takum<32>>` (compile-time via constexpr logs, C++20+). Shim deprecated M_PI with warning.
     8. Tapered analysis: Bounds via Proposition 11 (λ(p) < 2/3 ε(p)). Cover deprecated subnormals (no support, saturate with `std::expected`).
  9. Deprecations: Shims for all C-style funcs (e.g., #define sinf takum_sin_f with #pragma message); reverted C++26 (e.g., pow(NaN,0)=NaR). Pre-gen poly coeffs script for hybrid Φ.
     10. Gaussian-log Impl: LUT (1024/4096, 4-byte fixed-point entries, linear/cubic interp) for <64-bit; hybrid (256 LUT + minimax poly) for 64+; constexpr arrays for coeffs.
   - **Dependencies**: Phase 3 ops; LNS tables [mdpi.com](https://www.mdpi.com/2073-8994/16/9/1138).
   - **Milestones**: Match IEEE 754 accuracy within Takum's range; closure tests (Section 5.6); validate all deprecated funcs (e.g., sinf(float) → sin(takum32)); benchmark Φ latency (LUT vs. poly).
   - **Deliverables**: `takum/math.h`; FP example: `auto compose = [](auto f, auto g) { return [f,g](takum x) { return f(g(x)); }; }; auto sin_exp = compose(sin, exp);`.
   - **Files Created**:
     - `include/takum/math.h`: Overloads for trig (`sin`, `cos`, `tan`, `asin`, etc., with f/l variants shims), exp/log (`exp`, `log`, `log10`), power/root (`pow`, `sqrt`, `cbrt`, `hypot`), rounding/classify (`floor`, `ceil`, `round`, `fmod`, `isfinite`, `isnan`, `isinf`, `fpclassify`); constants (`pi_v<takum<N>>`); Gaussian log impl (LUT/hybrid with fixed-point interp, pre-gen poly coeffs); `std::expected` safe variants; full deprecated shims (e.g., `sinf(takum32 x)` with #pragma).
     - `include/takum/math_constants.h`: Shared file for `<numbers>`-like constants (e.g., `template<size_t N> constexpr takum<N> pi();`); deprecated M_PI shim with warning.
     - `test/math.test.cpp`: Tests for accuracy vs. IEEE (rel err ≤ λ(p), Prop 11), closure (Section 5.6: sqrt exactness > posits), all deprecated funcs (e.g., logl(long double) → log(takum128)), Φ approx (LUT/poly accuracy), `std::expected` paths.
     - `examples/math.cpp`: Example: `auto pipeline = std::views::transform(sin) | std::views::transform(exp);` including deprecated like `sinf(1.0f)` and hybrid Φ usage.
     - `docs/math.md`: Docs with tapered precision bounds (Props 9-11), deprecation migration, Φ details (LUT sizes/interp, poly degrees).

#### Phase 5: Standard Library Integrations (6 weeks, Medium Effort)
   - **Goals**: Replace FP in libs like `<complex>`, `<random>`, `<numeric>`, `<format>`. Cover deprecated STL (e.g., old valarray ops, C++98 chrono indirect). Use packed storage; `std::expected` for error-prone (e.g., random gen NaR); <stdfloat> full interop (conversions in core).
   - **Steps**:
     1. Complex: `template<size_t N> struct complex_takum { takum<N> real, imag; };` with ops (e.g., `complex_takum<N> sin(complex_takum<N> z);` using packed). Shim deprecated real/imag accessors with warnings.
     2. Random: `std::uniform_real_distribution<takum<N>>`, `std::normal_distribution<takum<N>>` (log-adapted with Φ hybrid). Cover deprecated engines (e.g., minstd_rand indirect); `std::expected` for invalid gen.
     3. Numerics: `std::accumulate<std::vector<takum<N>>>` (pure reduce with + on packed). All algos including deprecated partial_sum variants; parallel with execution.
  4. I/O/Format: `std::format("{:.2t}", takum x);` (C++20+); overload `operator<<` for `std::ostream`. Cover deprecated iomanip (e.g., old setprecision for long double with shims); <print> (C++26) overloads.
     5. Ranges/Algorithms: FP views e.g., `std::views::iota(takum{0}, takum{10}) | std::views::transform(sin);`. Cover deprecated algorithm overloads (e.g., C++98 sort on floats); `std::expected` for transform errors.
     6. Chrono/Atomics: `std::chrono::duration<takum<N>>` (fractional on packed); `std::atomic<takum<N>>`. Shim deprecated chrono (pre-C++11 fractional).
     7. Valarray: `std::valarray<takum<N>>` with slice/apply as pure funcs on packed. Cover all deprecated valarray ops (e.g., old sum with warnings).
     8. Charconv: Fast `to_chars`/`from_chars` for Takum packed. Cover deprecated sprintf for floats with shims.
     9. Bit: `std::bit_cast<takum<N>>` (from/to packed storage). Cover deprecated bit ops on floats.
     10. Execution: Parallel algos (e.g., `std::execution::par, std::transform_reduce`). Cover deprecated seq policy.
     11. Coroutines/Modules: Generic yield/return Takum (co_yield takum<N>); export as modules with packed.
     12. Deprecations: Shims for old STL (e.g., <valarray> legacy behaviors in compatibility.h with warnings).
     13. <stdfloat> Interop: Full conversions (e.g., std::float16_t → takum16 via encode); overloads where possible.
   - **Dependencies**: Phases 2-4.
   - **Milestones**: Compile/test STL-like code with Takum (e.g., vector ops); validate deprecated integrations (e.g., old valarray sum) and <stdfloat> (e.g., bfloat16_t to takum16).
   - **Deliverables**: `takum/stdlib.h`; example: Random Takum generation for ML sims [ieeexplore.ieee.org](https://ieeexplore.ieee.org/document/9458421).
   - **Files Created**:
     - `include/takum/complex.h`: `template<size_t N> struct complex_takum;` with ops (e.g., `complex_takum<N> sin(complex_takum<N> z);` on packed); deprecated shims (e.g., old conj with warning).
     - `include/takum/random.h`: `std::uniform_real_distribution<takum<N>>`, `std::normal_distribution<takum<N>>` (log-adapted with Φ); deprecated engine shims; `std::expected` for gen.
     - `include/takum/numeric.h`: `<numeric>` overloads (`accumulate`, `reduce`, `partial_sum` on packed); deprecated algos.
  - `include/takum/io.h`: `<format>`/`iostream` overloads (`std::format("{:.2t}", takum<N> x);`, `operator<<`); `<charconv>` (`to_chars(takum<N> x, char*);`); <print> (C++26); deprecated sprintf shim with warning.
     - `include/takum/ranges.h`: Ranges views/algos for Takum (e.g., `iota(takum<N>{0}, takum<N>{10})`); deprecated C++98 algos; `std::expected` transforms.
     - `include/takum/other.h`: Chrono (`duration<takum<N>>`), atomics (`std::atomic<takum<N>>`), valarray (`std::valarray<takum<N>>` on packed), bit (`bit_cast`), execution policies; deprecated chrono/valarray shims with warnings.
     - `include/takum/integrations/coroutines.h`: Coroutine support (e.g., co_yield takum<N>); deprecated async patterns shims.
     - `include/takum/integrations/modules.ixx`: C++20 module exports (packed types).
     - `test/stdlib.test.cpp`: Integration tests (e.g., random gen → accumulate → format); deprecated tests (e.g., old valarray); <stdfloat> interop.
     - `examples/stdlib.cpp`: Full STL example: Complex vector sort with Takum; deprecated legacy code; <stdfloat> example.
     - `docs/stdlib.md`: Migration guide for STL floating-point code, including deprecations and <stdfloat>.

#### Phase 6: Functional Programming Abstractions (4 weeks, Medium Effort)
   - **Goals**: Layer FP primitives on top for composability. Cover deprecated FP patterns (e.g., old bind1st for partial app). Use `std::expected` for monadic NaR; packed for efficiency.
   - **Steps**:
     1. Functors: `template<class Op> struct takum_op { Op op; takum apply(takum x, takum y) const; };` (on packed).
     2. Monads: `template<size_t N> struct takum_maybe { std::optional<takum<N>> val; };` for NaR (bind with pure funcs). Primary: `std::expected<takum<N>, takum_error>` with fallback (macro TAKUM_USE_OPTIONAL); `is_nar()`/`to_expected()` helpers.
  3. Pipelines: Use `std::ranges::pipeable` (C++26) for `takum x | sin | exp;`.
     4. Higher-order: `template<class F> auto map_f(F f) { return [f](auto cont) { return cont | std::views::transform([f](takum t){ return f(t); }); }; }`.
     5. Currying/Partial: Lambdas for partial app e.g., `auto add_five = std::bind(add, std::placeholders::_1, takum{5});`. Shim deprecated binders (bind1st) with warnings.
     6. Immutable containers: `std::vector<takum<N>>` with pure transforms on packed.
     7. Deprecations: Shim old FP (e.g., std::for_each with Takum lambdas via compatibility.h).
   - **Dependencies**: Phase 5.
   - **Milestones**: FP example replacing FP code: Neural net forward pass as composed funcs; test deprecated binders and `std::expected` monads.
   - **Deliverables**: `takum/functional.h`; benchmarks showing FP overhead <5%.
   - **Files Created**:
     - `include/takum/functional.h`: FP primitives (functors, monads like `takum_maybe<N>` with `std::expected` primary/fallback macro, pipelines via `pipeable`, higher-order `map_f`, currying binders); `is_nar()`/`to_expected()`; deprecated shims (e.g., bind1st analog with #pragma).
     - `test/functional.test.cpp`: Tests for purity (no side effects), monad laws, composition (e.g., sin ∘ exp); deprecated binder tests; `std::expected` paths.
     - `examples/functional.cpp`: FP neural net pass: `layer | map_f(relu) | reduce(add);` including old-style partial app and expected handling.
     - `docs/functional.md`: FP tutorial with lambdas/ranges, deprecation notes, NaR monads.

#### Phase 7: Testing and Validation (4-6 weeks, High Effort)
   - **Goals**: Ensure correctness vs. IEEE 754 (within Takum range) and paper props. 100% coverage including deprecations (e.g., test old sinf accuracy). Validate packed storage, Φ impls (LUT/poly), `std::expected`.
   - **Steps**:
     1. Unit: Google Test for encoding (Proposition 8 on packed), uniqueness (Prop 3: full τ, including sign S via high-precision long double decoded values for internal tuple (S, c, m) or ℓ), machine precision (Prop 11: |err| ≤ λ(p)). **Critical: ℓ vs τ in Tests**: Ensure uniqueness tests compare full τ (high-precision decoded canonical form, option A), not ℓ alone, to avoid false failures from sign-differing equal-ℓ cases; if using to_double(), allow <= or detect ties and compare canonical encodings (option B). Use long double to prevent rounding-induced ties/collapses.
     2. Closure: Exhaustive for N=8/16 (Section 5.6: exact mul/div > posits); include deprecated ops (e.g., old fmod); test Φ (LUT accuracy for 16/32, poly for 64+). **Critical: Ordering in Monotonicity Tests**: For Prop 4 monotonicity, use SI order iteration (start 1<<(n-1), wrap around); unsigned 0..2^n-1 assumes wrong correspondence to ascending τ, causing false monotonicity failures. Since NaR is special (canonical pattern, smallest value per total-ordering), skip neighbor comparisons across NaR or special-case it (e.g., check monotonicity separately for real values, excluding NaR boundary); compare using high-precision long double decoded values for internal tuple (S, c, m) or ℓ (option A) to avoid false ties from double rounding; or use to_double() with <= and tie-detection via canonical encodings (option B).
     3. Fuzz: Random doubles → Takum → ops → back, check rel err < ε(p); fuzz deprecated inputs (e.g., subnormals); include packed bit ops.
     4. Benchmarks: Dynamic range (Fig 4), waste ratio (Prop 2), vs. posits [dl.acm.org](https://dl.acm.org/doi/10.1145/3461699); test deprecated formats (float8); Φ perf (interp/poly).
     5. FP-specific: Test purity (no globals mutated), composition (e.g., monad laws with expected).
     6. Cross-precision: Conversion (Prop 8 flexibility); deprecated long double; <stdfloat>.
     7. Edge: NaR prop (Prop 9) via expected; saturation; all deprecations (e.g., pow(1,NaN)=1 reverted to NaR).
     8. Property-based: Hypothesis-like for all props (3-11); coverage for every feature/deprecation; packed vs. bitset debug.
   - **Dependencies**: All prior.
   - **Milestones**: 95% code coverage; 100% feature/deprecation coverage; validate against paper tables (e.g., Table 4 examples) using high-precision comparisons to guard against double rounding issues in tests; Φ benchmarks.
   - **Deliverables**: `test/` suite; report on golden zone (Fig 5-8).
   - **Files Created**:
     - `test/comprehensive.test.cpp`: Fuzz/integration suite (random Takum ops → err check); deprecated feature tests; packed storage validation.
     - `test/benchmark.test.cpp`: Closure analysis (Section 5.6), waste/dynamic range (Props 1-2 analogs); deprecated benchmarks; Φ LUT/poly perf.
     - `test/precision.test.cpp`: Mantissa bounds (Props 9-10), machine eps (Prop 11); deprecated precision (subnormals).
     - `test/deprecated.test.cpp`: Tests for all shims (e.g., sinf, old NaN); expected NaR.
     - `test/phi.test.cpp`: Gaussian-log tests (LUT interp for 16/32, poly for 64+).
     - `docs/validation.md`: Test report with coverage stats, benchmarks vs. IEEE/posits, deprecation validation, Φ details.
     - `scripts/run_benchmarks.py`: Script for perf validation (optional, for CI); include deprecated/Φ runs.
     - `scripts/gen_poly_coeffs.py`: Offline script for minimax poly coeffs (degree-5..7 for >32-bit).

#### Phase 8: Performance Optimization and Hardware Considerations (4 weeks, Medium Effort)
   - **Goals**: Optimize for speed/power, per paper (Section 5.2: simple LUT for regime). Cover deprecated perf (e.g., old non-optimized long double). Leverage packed storage for SIMD; optimize Φ (fixed-point interp, pre-gen poly).
   - **Steps**:
     1. LUTs: 16-byte table for regime parsing (3 entries/state on packed); Φ LUTs (1024/4096, fixed-point linear/cubic interp).
     2. SIMD: AVX intrinsics for vectorized add/mul (log domain shifts on uint64_t packed); optimize deprecated float ops.
  3. Inline/Constexpr: Mark pure funcs constexpr (C++26 relaxations for logs/polys).
     4. LNS Opts: Piecewise approx for Φ [ieeexplore.ieee.org](https://ieeexplore.ieee.org/document/8268821/); base √e shifts (Section 4.4); hybrid poly (256 LUT + minimax, constexpr coeffs).
     5. Profile: Compare latency/power vs. float64 (expect < IEEE for mul/div [mdpi.com](https://www.mdpi.com/2073-8994/16/9/1138)); include deprecated benchmarks; packed vs. bitset.
     6. Parallel: `<execution>` policies for Takum reductions on packed.
   - **Dependencies**: Phase 7.
   - **Milestones**: 2x speedup on mul-heavy workloads (e.g., matrix mul); test deprecated perf parity; Φ poly accuracy >99.9%.
   - **Deliverables**: Optimized builds; perf report.
   - **Files Created**:
     - `include/takum/optimized.h`: SIMD intrinsics (AVX for vector ops on packed), LUTs (regime/Φ), constexpr opts (poly coeffs); deprecated opt shims.
     - `test/perf.test.cpp`: Latency/power benchmarks (mul/add vs. float64); deprecated tests; Φ interp/poly vs. reference.
     - `docs/optimization.md`: Perf guide (e.g., LNS tables from [8268821]); deprecation perf notes, packed rationale.
     - `CMakeLists.txt` (updates): Add opt flags (e.g., `-march=native`); build poly coeffs from script.

#### Phase 9: Documentation and Examples (2-3 weeks, Low Effort)
   - **Goals**: User-friendly docs with FP examples. Cover deprecations in migration guide. Document packed storage, Φ impls, `std::expected`, compatibility warnings.
   - **Steps**:
     1. Doxygen: API docs for all funcs/types; detail packed, Φ (LUT sizes/poly), expected.
     2. Tutorials: Replace float code (e.g., sin loop → Takum pipeline); include deprecated migration (e.g., <math.h> → shims with warnings).
     3. Examples: ML (bfloat16-like [ieeexplore.ieee.org](https://ieeexplore.ieee.org/document/9458421/)), scientific (SI constants Table 5); deprecated legacy code; Φ usage.
     4. FP Focus: "Composing Takum Ops" section with lambdas/ranges; expected monads.
     5. Deprecations: Warn on usage, provide shims in compatibility.h (e.g., #pragma message("Deprecated: Use takum_sin. Legacy behavior: ...")).
   - **Dependencies**: All prior.
   - **Milestones**: Full README with benchmarks.
   - **Deliverables**: `docs/`; GitHub repo.
   - **Files Created**:
     - `docs/full_api.md`: Doxygen-generated API reference.
     - `docs/migration.md`: Guide for replacing float code (e.g., `double x;` → `takum64 x;`); full deprecation matrix.
     - `docs/compatibility.md`: Deprecated features guide/shims; warning examples.
     - `examples/advanced.cpp`: ML/sims (SI constants Table 5, cosmological const Table 6); deprecated example; Φ hybrid demo.
     - `examples/fp_demo.cpp`: Full FP pipeline for matrix mul with expected.
     - `CHANGELOG.md`: Version history.
     - `include/takum/compatibility.h`: Dedicated shims for all deprecations (e.g., #define sinf takum_sinf; #pragma message("Deprecated: sinf uses legacy C-style; prefer takum_sin. Behavior: maps to NaR on invalid.")); export <math.h> aliases (sinf/logl/M_PI) with warnings; old pow/NaN shims; sprintf fallbacks; portable warnings via #ifdef _MSC_VER / #pragma vs. #warning.

#### Phase 10: Release, Maintenance, and Extensions (Ongoing, Low Effort)
   - **Goals**: Publish and iterate. Cover future deprecations (e.g., C++26 proposals). Maintain packed/Φ/expected; update shims.
   - **Steps**:
     1. Package: CMake install; conan/vcpkg support.
  2. Extensions: Linear Takum toggle; C++26 `<stdfloat>` interop (full in core); deprecated migration tools (e.g., script to add shims).
     3. Community: RFC for standardization; contrib for more funcs (e.g., new Φ approx).
     4. Maintenance: Fix bugs, add C++26 support; monitor deprecations; update poly coeffs script.
   - **Dependencies**: Phase 9.
   - **Milestones**: v1.0 release; paper citation integration.
   - **Deliverables**: Releases; issue tracker.
   - **Files Created**:
  - `include/takum/extensions.h`: Linear Takum toggle (`template<bool Linear> struct takum_variant;`), `<stdfloat>` interop (C++26, e.g., promote(std::float128_t)); future deprecation shims.
     - `test/extensions.test.cpp`: Tests for linear variant (Definition 8); deprecated extensions; expected interop.
     - `docs/extensions.md`: Future work (e.g., C++26 support, deprecations); Φ updates.
     - `LICENSE`: MIT/Apache license.
  - `.github/workflows/ci.yml`: CI for tests/benchmarks (C++26, multi-compiler).
     - `conanfile.txt` / `vcpkg.json`: Packaging.

### Comprehensive Replacement Mapping
This section lists **100% of floating-point-related C++ features** (C++98 to C++26, including all deprecations/obsoletes). Chronological by standard for completeness. Replacements are pure/FP (immutable, composable). Organization: Precision files in `types/` (e.g., `takum32.h` for float); groups in subdirs (e.g., `math/trig.h`). Deprecations in `compatibility.h` with shims (e.g., #pragma message("Deprecated: Use takum_sin instead")).

#### Directory Structure Overview
```
include/takum/
├── core.h                          # Base takum<N>, literals (1.0_tk), concepts, bit_cast
├── types/
│   ├── precision_traits.h          # Traits/limits for all (λ(p), range); deprecated is_iec559=false
│   ├── takum8.h                    # Deprecated float8 (takum<8>)
│   ├── takum16.h                   # half/float16/bfloat16 (takum<16>)
│   ├── takum19.h                   # TF32 (takum<19>)
│   ├── takum32.h                   # float/float32 (takum<32>)
│   ├── takum64.h                   # double/float64 (takum<64>)
│   ├── takum128.h                  # long double/float128 (takum<128>)
│   └── takum256.h                  # Deprecated float256 (takum<256>)
├── arithmetic.h                    # Operators (+, -, *, /, abs, comparisons; bitwise)
├── math/
│   ├── constants.h                 # pi_v, e_v, M_PI shim
│   ├── trig.h                      # sin/cos/tan/asin/... (f/l variants)
│   ├── hyperbolic.h                # sinh/cosh/... 
│   ├── power.h                     # pow/sqrt/cbrt/hypot (deprecated pow(NaN,0))
│   ├── exp_log.h                   # exp/log/log10 (deprecated log(0))
│   └── rounding.h                  # floor/ceil/round/fmod/fpclassify (modes)
├── integrations/
│   ├── complex.h                   # complex_takum<N> (real/imag/conj)
│   ├── random.h                    # Distributions (uniform/normal)
│   ├── numeric.h                   # accumulate/reduce/partial_sum
│   ├── io.h                        # <<, format, to_chars (sprintf shim)
│   ├── ranges.h                    # iota/transform (C++98 algos)
│   ├── other.h                     # chrono duration/atomic/valarray/bit/execution
│   ├── coroutines.h                # co_yield takum
│   └── modules.ixx                 # Exports
├── functional.h                    # FP (monads/pipelines; bind1st shim)
├── optimized.h                     # SIMD/LUT
├── compatibility.h                 # All deprecations/shims (<math.h>, old NaN, subnormals)
└── extensions.h                    # Linear/<stdfloat> interop
```

#### Detailed Replacements List (Chronological by Standard)
- **C++98 (Initial)**:
  - Built-in Types: `float/double/long double` → `takum32/takum64/takum128` in `types/*.h`.
  - Literals: Decimal/scientific/suffixes (f/L) → User-defined in `core.h` (e.g., `1.0f` → `takum32{1.0}`).
  - Operators: + - * / unary comparisons % (deprecated for floats) → `arithmetic.h`.
  - <limits>: numeric_limits (min/max/epsilon/infinity/NaN) → `types/precision_traits.h`.
  - <cmath>: All basic (sin/cos/exp/log/sqrt/floor/fabs/fmod/hypot) + deprecated <math.h> aliases (sinf/sinl) → `math/*.h`; shims in `compatibility.h`.
  - <complex>: std::complex<T> ops/real/imag/abs/arg/conj/sin → `integrations/complex.h`.
  - <valarray>: valarray<T>/slice/apply/sum → `integrations/other.h`; deprecated ops shims.
  - <iostream>/<iomanip>: << / fixed/scientific/setprecision → `integrations/io.h`; deprecated manip shims.
  - <numeric>: accumulate/inner_product/partial_sum/adjacent_difference → `integrations/numeric.h`.
  - Containers/Algorithms: vector<>/sort/transform on floats → Generic + `ranges.h` for C++20 views; deprecated algos shims.
- **C++03 (Minor)**: Refined limits/behaviors → Covered in traits (Phase 2).
- **C++11**:
  - Hex Literals: 0x1.2p3 → `core.h` (user-defined hex_tk).
  - Type Traits: is_floating_point/is_iec559 → `types/precision_traits.h`.
  - <random>: Engines/distributions (uniform_real/normal) → `integrations/random.h`.
  - <chrono>: duration (fractional) → `integrations/other.h`.
  - <atomic>: atomic<double> → `integrations/other.h`.
  - constexpr Basics: Float ops in constexpr → All pure funcs constexpr in respective .h.
  - <cstdint> Fixed-Width: Indirect bits (uint32_t for float bits) → `core.h` bit_cast.
  - <string> to_string(double) → `io.h` to_string(takum<N>).
  - Enhanced <cmath>: cbrt/hypot/isfinite/isnan → `math/power.h`/`rounding.h`; deprecated classification.
- **C++14**: Constexpr Enhancements (more float ops) → Extended constexpr in all .h.
- **C++17**:
  - <charconv>: to_chars/from_chars (float) → `integrations/io.h`.
  - <execution>: par/reduce/transform_reduce → `integrations/numeric.h`/`other.h`.
  - <filesystem>: Indirect paths with float coords → Generic (use to_string).
  - std::gcd/lcm: Integer, but cast to Takum contexts → `arithmetic.h` shim.
  - Enhanced <numeric>: reduce/transform_reduce → Covered.
- **C++20**:
  - Concepts: floating_point → `core.h` takum_floating_point.
  - <format>: "{:.2f}" → `integrations/io.h`.
  - <numbers>: pi_v/e_v<T> → `math/constants.h`.
  - <bit>: bit_cast (floats) → `core.h`.
  - <ranges>: accumulate/iota/transform on floats → `integrations/ranges.h`.
  - Coroutines: Yield floats → `integrations/coroutines.h`.
  - Modules: Export float funcs → `integrations/modules.ixx`.
  - Unevaluated Contexts: Full constexpr floats → Extended.
- **C++26**:
  - <stdfloat>: float16_t/bfloat16_t/etc. → `extensions.h` interop (convert to takum<N>).
  - <print>/println: Print floats → `integrations/io.h`.
  - Enhanced <cmath>/<numeric>/<format>/<charconv>: Overloads for extended types → All .h with Takum.
  - Obsoletes/Deprecations: Reverted <cmath> (e.g., pow(NaN,0)=1 → NaR); no subnormals in bfloat16 → Saturation shims in `compatibility.h`.
  - Defect Reports: Edge NaN/precision → Tested in Phase 7.

This ensures 100% coverage: Every feature has a replacement/shim, organized for builds (e.g., include `compatibility.h` for legacy).