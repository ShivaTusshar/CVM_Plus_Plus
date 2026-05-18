# CVM++

A small statically-typed scripting language that compiles to bytecode and
runs on a stack-based VM. Modern C++17, single-binary, ~1500 LOC.

Architecture follows the _Crafting Interpreters_ model: source text →
tokens → AST → bytecode → VM execution, with each function carrying its
own `Chunk` (a flat `std::vector<uint8_t>` of instructions plus a small
per-function constant pool and global-name table).

## Build

```bash
mkdir build && cd build
cmake .. && cmake --build .
```

Or a single command, no CMake:

```bash
g++ -std=c++17 -O2 main.cpp -o cvmpp
```

## Usage

```bash
./cvmpp                              # REPL
```
```bash
./cvmpp fizzbuzz.cvm        # run a script
```
```bash
./cvmpp --debug factorial.cvm   # disassemble bytecode, then run
```
```bash
cd ..
./build/cvmpp fizzbuzz.cvm.     #run a script after cmake
```

## Language

```
// Types: Int, Bool, Nil, Function
let n = 10;                  // global at top level, local inside any block
let ok = true;
let nothing = nil;

// Arithmetic and comparison
print 2 + 3 * 4;             // 14
print 10 / 3;                // 3 (integer division)
print 1 < 2;                 // true
print !ok;                   // false

// Control flow
if (0 < n && n < 100) { print n; } else { print 0; }

while (0 < n) { n = n - 1; }

for (let i = 0; i < 5; i = i + 1) {
    if (i == 3) { continue; }
    if (5 < i)  { break; }
    print i;
}

// Functions — top-level, first-class values, recursion via global lookup
fn factorial(n) {
    if (n < 2) { return 1; }
    return n * factorial(n - 1);
}
print factorial(10);         // 3628800

// I/O
print "type a number";       // (strings not supported — use print of value)
let x = input;               // reads one integer from stdin
print x * x;
```

### Operators (lowest → highest precedence)

| Operator  | Notes                                   |
| --------- | --------------------------------------- |
| `=`       | assignment (only to declared variables) |
| `\|\|`    | short-circuit logical OR                |
| `&&`      | short-circuit logical AND               |
| `==`      | equality                                |
| `<`       | less-than                               |
| `+` `-`   | addition / subtraction                  |
| `*` `/`   | multiplication / integer division       |
| `-` `!`   | unary negate / logical NOT              |
| `f(args)` | function call                           |

Conditions in `if` / `while` / `for` / `&&` / `||` must be `Bool` —
there is no implicit truthiness (`if (5) { … }` is a runtime error).

### Scopes

- `let` at the top level → **global** (stored in a runtime hashmap).
- `let` inside any `{ … }` or function body → **local** (a stack slot
  resolved at compile time, accessed by `OP_GET_LOCAL`/`OP_SET_LOCAL`).

Locals shadow outer variables of the same name and die at the closing
brace. Duplicate `let` in the same scope is a compile error.

### Functions

- Declared with `fn name(p1, p2, …) { body }`. Up to 32 params.
- Always installed as **globals**, so any function can call any other
  declared-before-it (including itself — recursion works trivially).
- No closures: a function body sees its own params/locals plus globals,
  not enclosing locals.
- Functions are first-class `Value`s — `let g = factorial; g(5);` works.
- `return expr;` returns a value; `return;` or falling off the end
  returns `nil`.

## Project layout

```
opcodes.hpp     One-byte opcode definitions
value.hpp       Tagged Value (Nil, Int, Bool, Function)
chunk.hpp       Bytecode + constant pool + line table
function.hpp    Function = name + arity + Chunk + names pool
token.hpp       Token kinds
lexer.hpp       Source → Token stream
ast.hpp         AST node definitions
parser.hpp      Recursive-descent parser
compiler.hpp    AST → bytecode (locals, scopes, loops, fns)
debug.hpp       Bytecode disassembler
vm.hpp          Stack-based interpreter w/ call frames
main.cpp        REPL + file runner
examples/       Working .cvm programs (see below)
```

## Examples

| File            | Feature                                          |
| --------------- | ------------------------------------------------ |
| `demo.cvm`      | arithmetic, comparison, if/else                  |
| `fizzbuzz.cvm`  | classic FizzBuzz via `while`                     |
| `sum.cvm`       | `input`, accumulate 1..N                         |
| `scope.cvm`     | block-scoped locals shadowing a global           |
| `for_loop.cvm`  | `for` with loop-local index, nested loops        |
| `factorial.cvm` | recursive function                               |
| `fibonacci.cvm` | naive recursion + iterative-with-locals          |
| `control.cvm`   | `break`, `continue`, short-circuit `&&` / `\|\|` |

## VM internals at a glance

- **Stack-based dispatch.** A `switch` in `vm.hpp::dispatch()` decodes
  one opcode per iteration. Operands (locals slot, jump offset, name
  index) follow inline.
- **Call frames.** A 256-slot `CallFrame[]` records, per active function,
  `(function*, ip, slots_base)`. Locals are addressed off `slots_base`
  so the VM never has to copy arguments on a call — they stay where the
  caller pushed them.
- **Jumps.** 16-bit big-endian relative offsets, back-patched at compile
  time once the target offset is known. `OP_LOOP` is the same encoding
  but interpreted as negative.
- **Globals.** A single `unordered_map<string, Value>` keyed by name.
  Names are interned per-function so bytecode only carries 1-byte
  indices into the function's `names` pool.
- **No GC.** All `Function`s are owned by the `Compiler`'s
  `unique_ptr<Function>` vector, which the VM borrows for the run.
