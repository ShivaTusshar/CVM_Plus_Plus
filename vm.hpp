#pragma once
#include "chunk.hpp"
#include "opcodes.hpp"
#include "value.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace cvm {

// Stack-based execution engine. The dispatch loop is a tight switch over a
// single byte fetched from the instruction stream, mimicking a simplified
// RISC fetch-decode-execute pipeline. We keep the stack as a fixed-size
// array with a hand-managed top pointer to avoid std::vector overhead in
// the hot path.
class VM {
public:
    static constexpr size_t STACK_MAX = 1 << 14; // 16K slots

    enum class Result { OK, RUNTIME_ERROR };

    Result run(const Chunk& chunk, const std::vector<std::string>& globalNames) {
        this->names = &globalNames;
        ip       = chunk.code.data();
        end      = ip + chunk.code.size();
        codeBase = chunk.code.data();
        stackTop = stack.data();
        constants = chunk.constants.data();

        try {
            dispatch();
        } catch (const std::exception& ex) {
            size_t at = (ip > codeBase) ? (size_t)(ip - codeBase) - 1 : 0;
            int line = (at < chunk.lines.size()) ? chunk.lines[at] : -1;
            std::cerr << "[line " << line << "] Runtime error: " << ex.what() << "\n";
            return Result::RUNTIME_ERROR;
        }
        return Result::OK;
    }

private:
    // ---- machine state ----
    std::array<Value, STACK_MAX> stack;
    Value*               stackTop = nullptr;
    const uint8_t*       ip       = nullptr;
    const uint8_t*       end      = nullptr;
    const uint8_t*       codeBase = nullptr;
    const Value*         constants = nullptr;
    const std::vector<std::string>* names = nullptr;
    std::unordered_map<std::string, Value> globals;

    // ---- stack helpers ----
    inline void  push(Value v)       { *stackTop++ = v; }
    inline Value pop()               { return *--stackTop; }
    inline Value& peek(int distance) { return *(stackTop - 1 - distance); }

    // ---- byte-stream helpers ----
    inline uint8_t  readByte()  { return *ip++; }
    inline uint16_t readShort() { ip += 2; return (uint16_t)((ip[-2] << 8) | ip[-1]); }
    inline Value    readConst() { return constants[readByte()]; }

    // ---- type guards ----
    static void requireInt(const Value& v, const char* op) {
        if (!v.isInt()) throw std::runtime_error(std::string("Operand to '") + op + "' must be Int.");
    }

    // ---- dispatch loop ----
    void dispatch() {
        for (;;) {
            if (ip >= end) throw std::runtime_error("Instruction pointer ran past end of chunk.");
            uint8_t instr = readByte();
            switch (instr) {
                case OP_CONSTANT: push(readConst()); break;
                case OP_TRUE:     push(Value::makeBool(true));  break;
                case OP_FALSE:    push(Value::makeBool(false)); break;

                case OP_POP:      (void)pop(); break;

                case OP_DEFINE_GLOBAL: {
                    uint8_t idx = readByte();
                    globals[(*names)[idx]] = pop();
                    break;
                }
                case OP_GET_GLOBAL: {
                    uint8_t idx = readByte();
                    const std::string& n = (*names)[idx];
                    auto it = globals.find(n);
                    if (it == globals.end())
                        throw std::runtime_error("Undefined variable '" + n + "'.");
                    push(it->second);
                    break;
                }
                case OP_SET_GLOBAL: {
                    uint8_t idx = readByte();
                    const std::string& n = (*names)[idx];
                    auto it = globals.find(n);
                    if (it == globals.end())
                        throw std::runtime_error("Undefined variable '" + n + "'.");
                    it->second = peek(0);     // leave value on stack (expression result)
                    break;
                }

                case OP_ADD: { Value b = pop(); Value a = pop();
                    requireInt(a, "+"); requireInt(b, "+");
                    push(Value::makeInt(a.as.i + b.as.i)); break; }
                case OP_SUB: { Value b = pop(); Value a = pop();
                    requireInt(a, "-"); requireInt(b, "-");
                    push(Value::makeInt(a.as.i - b.as.i)); break; }
                case OP_MUL: { Value b = pop(); Value a = pop();
                    requireInt(a, "*"); requireInt(b, "*");
                    push(Value::makeInt(a.as.i * b.as.i)); break; }
                case OP_DIV: { Value b = pop(); Value a = pop();
                    requireInt(a, "/"); requireInt(b, "/");
                    if (b.as.i == 0) throw std::runtime_error("Division by zero.");
                    push(Value::makeInt(a.as.i / b.as.i)); break; }
                case OP_NEGATE: {
                    Value a = pop(); requireInt(a, "-");
                    push(Value::makeInt(-a.as.i)); break; }

                case OP_EQUAL: { Value b = pop(); Value a = pop();
                    push(Value::makeBool(a.equals(b))); break; }
                case OP_LESS: { Value b = pop(); Value a = pop();
                    requireInt(a, "<"); requireInt(b, "<");
                    push(Value::makeBool(a.as.i < b.as.i)); break; }
                case OP_NOT: {
                    Value a = pop();
                    if (!a.isBool()) throw std::runtime_error("Operand to '!' must be Bool.");
                    push(Value::makeBool(!a.as.b)); break; }

                case OP_JUMP: { uint16_t off = readShort(); ip += off; break; }
                case OP_JUMP_IF_FALSE: {
                    uint16_t off = readShort();
                    Value& v = peek(0);
                    if (!v.isBool())
                        throw std::runtime_error("Condition must be Bool.");
                    if (!v.as.b) ip += off;
                    break;
                }
                case OP_LOOP: { uint16_t off = readShort(); ip -= off; break; }

                case OP_PRINT: { Value v = pop(); std::cout << v << "\n"; break; }
                case OP_INPUT: {
                    int64_t x;
                    if (!(std::cin >> x))
                        throw std::runtime_error("Failed to read integer from stdin.");
                    push(Value::makeInt(x));
                    break;
                }

                case OP_HALT: return;

                default:
                    throw std::runtime_error("Unknown opcode: " + std::to_string(instr));
            }
        }
    }
};

} // namespace cvm
