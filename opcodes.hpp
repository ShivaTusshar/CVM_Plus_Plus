#pragma once
#include <cstdint>

namespace cvm {

// Bytecode instruction set for CVM++ (stack-based VM).
// One-byte opcodes; operands (if any) follow inline.
enum OpCode : uint8_t {
    // ---- Constants & Literals ----
    OP_CONSTANT,        // [op, idx]            push constants[idx]
    OP_NIL,             // [op]                 push nil
    OP_TRUE,            // [op]                 push true
    OP_FALSE,           // [op]                 push false

    // ---- Stack ----
    OP_POP,             // [op]                 discard top

    // ---- Locals (stack slots, resolved at compile time) ----
    OP_GET_LOCAL,       // [op, slot]           push frame.slots[slot]
    OP_SET_LOCAL,       // [op, slot]           frame.slots[slot] = peek(0)

    // ---- Globals (name lookup at runtime) ----
    OP_DEFINE_GLOBAL,   // [op, name_idx]       globals[name] = pop
    OP_GET_GLOBAL,      // [op, name_idx]       push globals[name]
    OP_SET_GLOBAL,      // [op, name_idx]       globals[name] = peek(0)

    // ---- Arithmetic ----
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_NEGATE,

    // ---- Comparison / Logical ----
    OP_EQUAL, OP_LESS, OP_NOT,

    // ---- Control flow (16-bit big-endian operand) ----
    OP_JUMP,            // [op, hi, lo]         ip += offset
    OP_JUMP_IF_FALSE,   // [op, hi, lo]         peek; if false ip += offset
    OP_JUMP_IF_TRUE,    // [op, hi, lo]         peek; if true  ip += offset  (for ||)
    OP_LOOP,            // [op, hi, lo]         ip -= offset (backward)

    // ---- Calls ----
    OP_CALL,            // [op, argCount]       call fn at stack[-argCount-1]
    OP_RETURN,          // [op]                 pop value, unwind frame

    // ---- I/O ----
    OP_PRINT,           // [op]                 pop and print
    OP_INPUT            // [op]                 read int from stdin, push as Int
};

} // namespace cvm
