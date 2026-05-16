#pragma once
#include <cstdint>

namespace cvm {

// Bytecode instruction set for the CVM++ stack-based VM.
// Each opcode is exactly one byte. Operands (if any) follow immediately in the stream.
enum OpCode : uint8_t {
    // --- Constants & Literals ---
    OP_CONSTANT,        // [op, idx]            push constants[idx] onto stack
    OP_TRUE,            // [op]                 push BOOL(true)
    OP_FALSE,           // [op]                 push BOOL(false)

    // --- Stack management ---
    OP_POP,             // [op]                 pop and discard top of stack

    // --- Globals (variables live in a hash table keyed by name index) ---
    OP_DEFINE_GLOBAL,   // [op, name_idx]       pop value, define global with name names[name_idx]
    OP_GET_GLOBAL,      // [op, name_idx]       push value of global
    OP_SET_GLOBAL,      // [op, name_idx]       assign top-of-stack to existing global (leaves value on stack)

    // --- Arithmetic ---
    OP_ADD,             // [op]                 b=pop,a=pop, push a+b
    OP_SUB,             // [op]                 b=pop,a=pop, push a-b
    OP_MUL,             // [op]                 b=pop,a=pop, push a*b
    OP_DIV,             // [op]                 b=pop,a=pop, push a/b
    OP_NEGATE,          // [op]                 a=pop,        push -a

    // --- Comparison / Logical ---
    OP_EQUAL,           // [op]                 b=pop,a=pop, push a==b   (BOOL)
    OP_LESS,            // [op]                 b=pop,a=pop, push a<b    (BOOL)
    OP_NOT,             // [op]                 a=pop,        push !a    (BOOL)

    // --- Control flow (16-bit operand stored big-endian) ---
    OP_JUMP,            // [op, hi, lo]         ip += offset
    OP_JUMP_IF_FALSE,   // [op, hi, lo]         peek top, if false ip += offset
    OP_LOOP,            // [op, hi, lo]         ip -= offset (backward jump)

    // --- I/O ---
    OP_PRINT,           // [op]                 pop and print
    OP_INPUT,           // [op]                 read int from stdin, push as INT

    // --- Halt ---
    OP_HALT             // [op]                 terminate execution
};

} // namespace cvm
