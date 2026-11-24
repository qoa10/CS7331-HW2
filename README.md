# CS7331 Homework 2: LLVM

This project implements two Clang LibTools for function analysis
and source-to-source operator strength reduction.

## Build

Tested on brooks with LLVM/Clang 16.0.4 and CMake 3.22.1.

```sh
L=/home/Faculty/aq10/llvm-project

cmake -S . -B build \
    -DLLVM_DIR="$L/build/lib/cmake/llvm" \
    -DClang_DIR="$L/build/lib/cmake/clang" \
    -DCMAKE_CXX_COMPILER="$L/build/bin/clang++" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build -j1
```

The Clang resource directory is detected automatically during
CMake configuration.

## Function Analyzer

```sh
./build/func-analyzer tests/test_analysis.c -- -std=c11
```

The tool reports the following information for each function
definition in the main source file:

- Function name
- Number of arguments
- Number of statements
- Number of loops
- Number of direct call sites

Function identity is determined using canonical declarations.
Statement counts exclude compound blocks and expression subtrees.
Loop counts include for, while, do-while and range-based for loops.

### Example Output

```text
Function name: helper
Number of arguments: 1
Number of statements: 1
Number of loops: 0
Number of calls: 2
```

### Limitations

- Call counts represent static direct call sites rather than runtime
  invocation counts.
- Indirect calls and implicit constructor calls are not counted.
- Nested lambda bodies are excluded from the enclosing function's
  body statistics.
- Template instantiations are not visited by default.
- Compiler-generated AST nodes may affect statement counts in
  complex C++ constructs.

## Operator Strength Reduction

```sh
./build/strength-reduction tests/test_transform.c -- -std=c11 > transformed.c
```

The tool replaces eligible integer multiplication and division by
positive power-of-two integer literals with shift operations.
Multiplication also supports a constant on the left.

Signed multiplication uses an unsigned intermediate when needed
to avoid undefined behavior from negative signed left shifts.
Signed division uses a rounding correction when the operand may
be negative.

The transformed source is written to standard output without
modifying the original file.

### Examples

```c
/* Before */
unsigned multiply(unsigned x) { return x * 4; }
unsigned divide(unsigned x) { return x / 2; }

/* After */
unsigned multiply(unsigned x) { return (x << 2); }
unsigned divide(unsigned x) { return (x >> 1); }
```

Signed multiplication example:

```c
/* Before */
int multiply(int x) { return x * 4; }

/* After */
int multiply(int x) {
    return ((int)((unsigned int)(x) << 2));
}
```

### Limitations

- Signed transformations rely on the integer representation,
  conversion behavior and arithmetic right shift of the tested
  target platform.
- Signed division requiring rounding correction is skipped when
  the operand has detected side effects.
- Only explicit integer literals are accepted as power-of-two
  constants. Expressions involving sizeof, named constants and
  other computed constants are not folded.
- Compound assignments, floating-point operations, overloaded
  operators and macro expressions are not supported.
- Overlapping expressions are handled innermost first. Additional
  passes may be required for nested expressions.
- Multiplication and division by one are left unchanged.
- Use one source file per transformation invocation.
- Invalid input may produce partial output with a nonzero exit code.

## Tests

| File | Purpose |
| --- | --- |
| test_analysis.c | Basic function analysis |
| test_analysis_cpp.cpp | Overloads, recursion and nested loops |
| test_transform.c | Basic operator transformations |
| test_signed.c | Signed integers and integer boundary cases |
| test_special.c | Side effects and volatile operands |
| test_headers.c | Standard library header compatibility |

### Analysis Tests

```sh
./build/func-analyzer tests/test_analysis.c -- -std=c11
./build/func-analyzer tests/test_analysis_cpp.cpp -- -std=c++17
./build/func-analyzer tests/test_headers.c -- -std=c11
```

### Transformation Tests

```sh
for test in test_transform test_signed test_headers; do
    ./build/strength-reduction "tests/$test.c" -- -std=c11 \
        > "/tmp/${test}_final.c" &&
    clang -std=c11 -Wall -Wextra -O0 \
        -fsanitize=undefined \
        -fno-sanitize-recover=undefined \
        "/tmp/${test}_final.c" -o "/tmp/${test}_final" &&
    "/tmp/${test}_final"
    echo "Result: $?"
done
```

All three transformation tests returned exit code 0 on brooks,
with no reported undefined behavior.

The Function Analyzer was also tested successfully on the original
NPB CG benchmark from the course repository.
