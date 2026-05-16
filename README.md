# CVM++

A lightweight scripting language that compiles to proprietary bytecode and runs on a custom stack-based virtual machine — written in modern C++17. Architecture follows *Crafting Interpreters* by Robert Nystrom.

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

This produces a single `cvmpp` executable.

## Usage

```bash
./cvmpp                            # interactive REPL
./cvmpp script.cvm                 # run a script file
./cvmpp --debug script.cvm         # disassemble bytecode, then run

./cvmpp ../examples/demo.cvm
./cvmpp ../examples/fizzbuzz.cvm
echo 100 | ./cvmpp ../examples/sum.cvm
```

## Language

- **Types**: `Int` (64-bit), `Bool`
- **Operators**: `+ - * /`, `==`, `<`, unary `-`, `!`
- **Variables**: `let x = 10;` then `x = x + 1;`
- **Control flow**: `if (cond) { ... } else { ... }`, `while (cond) { ... }`
- **I/O**: `print expr;` and the `input` keyword (reads an integer from stdin)
- **Comments**: `//` to end of line

## Architecture

| File           | Responsibility                                           |
| -------------- | -------------------------------------------------------- |
| `opcodes.hpp`  | Bytecode instruction set (one-byte opcodes)              |
| `value.hpp`    | Tagged-union `Value` (Int / Bool)                        |
| `chunk.hpp`    | Bytecode + constant pool container                       |
| `token.hpp`    | Token types and struct                                   |
| `lexer.hpp`    | Source string → tokens                                   |
| `ast.hpp`      | Expression and statement nodes                           |
| `parser.hpp`   | Recursive-descent parser, tokens → AST                   |
| `compiler.hpp` | AST → flat `std::vector<uint8_t>` bytecode               |
| `debug.hpp`    | Bytecode disassembler                                    |
| `vm.hpp`       | Stack VM with tight `switch` dispatch loop               |
| `main.cpp`     | CLI: file runner + REPL                                  |

The VM uses a fixed-size 16K-slot stack with a hand-managed top pointer and reads the instruction stream linearly from a `const uint8_t*`, giving a fetch–decode–execute path with minimal overhead.
