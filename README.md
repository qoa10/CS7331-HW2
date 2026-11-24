# CS7331 Homework 2: LLVM

## Build

On brooks, use the course LLVM 16 installation:

```sh
L=/home/Faculty/aq10/llvm-project
cmake -S . -B build -DLLVM_DIR="$L/build/lib/cmake/llvm" -DClang_DIR="$L/build/lib/cmake/clang" \
  -DCMAKE_CXX_COMPILER="$L/build/bin/clang++" -DCMAKE_C_COMPILER="$L/build/bin/clang" \
  -DCMAKE_CXX_FLAGS="-I$L/clang/include -I$L/build/tools/clang/include -I$L/llvm/include -I$L/build/include"
cmake --build build -j2
```

## Function Analyzer

`./build/func-analyzer tests/test_analysis.c -- -std=c11`

Reports function definitions, parameter counts, statement counts, loops and direct call sites in the main source file. Function identity uses canonical declarations. Statement counts exclude compound blocks and expression subtrees; loop counts include for, while, do-while and range-for.

### Limitations

- Calls are static direct call sites, not runtime invocation counts. Indirect calls and implicit constructor calls are not counted.
- Nested lambda bodies are excluded from the enclosing function's body statistics. Template instantiations are not visited by default.
- AST-generated constructs can affect statement counts in complex C++ code.

## Strength Reduction

```sh
./build/strength-reduction tests/test_transform.c -- -std=c11 > transformed.c
cc tests/test_transform.c -o original
cc transformed.c -o transformed
./original && ./transformed
```

Replaces integer multiplication and eligible division by positive powers of two with shifts. Multiplication also accepts a constant on the left. A cast is inserted when the promoted operand type differs from the original arithmetic result type. Output is written to stdout; the input file is unchanged.

### Limitations

- Signed multiplication is rewritten assuming a nonnegative operand and a representable result. Negative signed left shifts can be undefined in C11; this transformation is not generally equivalent for negative inputs.
- Signed division is skipped unless the operand is provably nonnegative, because right shift of negative integers can round differently from division.
- Compound assignments, floating-point operations, overloaded operators, macros and non-power-of-two constants are not rewritten.
- Overlapping expressions are handled innermost first; run again to rewrite eligible enclosing expressions.
- Multiplication or division by one is left unchanged.
- Use one source file per transformation invocation.
