#pragma once
#include "chunk.hpp"
#include "function.hpp"
#include "opcodes.hpp"
#include "value.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace cvm {

// Stack-based execution engine.
//   * `stack`  : Value array, hand-managed top pointer.
//   * `frames` : CallFrame array (function, ip, slots base).
// The dispatch loop is a tight switch over one fetched byte. Locals are
// just stack slots indexed off the current frame.
class VM {
public:
    static constexpr size_t STACK_MAX  = 1 << 14;   // 16K Values
    static constexpr size_t FRAMES_MAX = 256;       // call depth

    enum class Result { OK, RUNTIME_ERROR };

    Result run(const Function* mainFn) {
        if (!mainFn) return Result::RUNTIME_ERROR;
        stackTop  = stack.data();
        frameCnt  = 0;

        // Push the script as call frame 0. Slot 0 must hold a value (the
        // function itself) so locals[0] is well-defined.
        push(Value::makeFunction(mainFn));
        pushFrame(mainFn, stackTop - 1);

        try {
            dispatch();
        } catch (const std::exception& ex) {
            reportRuntimeError(ex.what());
            return Result::RUNTIME_ERROR;
        }
        return Result::OK;
    }

private:
    struct CallFrame {
        const Function* function;
        const uint8_t*  ip;
        Value*          slots;   // points at function value; locals at slots[1..]
    };

    std::array<Value, STACK_MAX>          stack;
    Value*                                stackTop = nullptr;
    std::array<CallFrame, FRAMES_MAX>     frames;
    size_t                                frameCnt = 0;
    std::unordered_map<std::string, Value> globals;

    // ---- stack helpers ----
    inline void  push(Value v)        { *stackTop++ = v; }
    inline Value pop()                { return *--stackTop; }
    inline Value& peek(int distance)  { return *(stackTop - 1 - distance); }

    CallFrame& frame() { return frames[frameCnt - 1]; }

    // ---- frame management ----
    void pushFrame(const Function* fn, Value* slots) {
        if (frameCnt == FRAMES_MAX) throw std::runtime_error("Stack overflow (call depth).");
        CallFrame& fr = frames[frameCnt++];
        fr.function = fn;
        fr.ip       = fn->chunk.code.data();
        fr.slots    = slots;
    }

    // ---- bytecode fetch ----
    inline uint8_t  readByte()  { return *frame().ip++; }
    inline uint16_t readShort() {
        frame().ip += 2;
        return static_cast<uint16_t>((frame().ip[-2] << 8) | frame().ip[-1]);
    }
    inline Value    readConst() { return frame().function->chunk.constants[readByte()]; }
    inline const std::string& readName(uint8_t idx) {
        return frame().function->names[idx];
    }

    // ---- type guards ----
    static void requireInt(const Value& v, const char* op) {
        if (!v.isInt())
            throw std::runtime_error(std::string("Operand to '") + op + "' must be Int.");
    }

    // ---- runtime error w/ call-stack trace ----
    void reportRuntimeError(const std::string& msg) {
        std::cerr << "Runtime error: " << msg << "\n";
        for (int i = static_cast<int>(frameCnt) - 1; i >= 0; --i) {
            const CallFrame& fr = frames[i];
            size_t at = (fr.ip > fr.function->chunk.code.data())
                        ? static_cast<size_t>(fr.ip - fr.function->chunk.code.data()) - 1 : 0;
            int line = (at < fr.function->chunk.lines.size()) ? fr.function->chunk.lines[at] : -1;
            std::cerr << "  [line " << line << "] in "
                      << (fr.function->name.empty() ? "<script>" : fr.function->name) << "\n";
        }
    }

    // ---- main dispatch loop ----
    void dispatch() {
        for (;;) {
            uint8_t instr = readByte();
            switch (instr) {
                case OP_CONSTANT: push(readConst()); break;
                case OP_NIL:      push(Value::makeNil()); break;
                case OP_TRUE:     push(Value::makeBool(true)); break;
                case OP_FALSE:    push(Value::makeBool(false)); break;
                case OP_POP:      (void)pop(); break;

                case OP_GET_LOCAL: {
                    uint8_t slot = readByte();
                    push(frame().slots[slot]);
                    break;
                }
                case OP_SET_LOCAL: {
                    uint8_t slot = readByte();
                    frame().slots[slot] = peek(0);
                    break;
                }

                case OP_DEFINE_GLOBAL: {
                    const std::string& n = readName(readByte());
                    globals[n] = pop();
                    break;
                }
                case OP_GET_GLOBAL: {
                    const std::string& n = readName(readByte());
                    auto it = globals.find(n);
                    if (it == globals.end())
                        throw std::runtime_error("Undefined variable '" + n + "'.");
                    push(it->second);
                    break;
                }
                case OP_SET_GLOBAL: {
                    const std::string& n = readName(readByte());
                    auto it = globals.find(n);
                    if (it == globals.end())
                        throw std::runtime_error("Undefined variable '" + n + "'.");
                    it->second = peek(0);
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

                case OP_JUMP: { uint16_t off = readShort(); frame().ip += off; break; }
                case OP_JUMP_IF_FALSE: {
                    uint16_t off = readShort();
                    const Value& v = peek(0);
                    if (!v.isBool()) throw std::runtime_error("Condition must be Bool.");
                    if (!v.as.b) frame().ip += off;
                    break;
                }
                case OP_JUMP_IF_TRUE: {
                    uint16_t off = readShort();
                    const Value& v = peek(0);
                    if (!v.isBool()) throw std::runtime_error("Condition must be Bool.");
                    if (v.as.b) frame().ip += off;
                    break;
                }
                case OP_LOOP: { uint16_t off = readShort(); frame().ip -= off; break; }

                case OP_CALL: {
                    uint8_t argc = readByte();
                    Value callee = peek(argc);
                    if (!callee.isFunction())
                        throw std::runtime_error("Can only call functions.");
                    const Function* fn = callee.as.fn;
                    if (argc != fn->arity)
                        throw std::runtime_error(
                            "Expected " + std::to_string(fn->arity) +
                            " argument(s), got " + std::to_string(argc) + ".");
                    // New frame's slot 0 points at the function value sitting
                    // beneath the arguments. Args become slots 1..argc.
                    pushFrame(fn, stackTop - argc - 1);
                    break;
                }

                case OP_RETURN: {
                    Value result = pop();
                    Value* slots = frame().slots;
                    --frameCnt;
                    if (frameCnt == 0) {
                        // Implicit return from <script>: we're done.
                        return;
                    }
                    stackTop = slots;       // drop fn-value + args + locals
                    push(result);
                    break;
                }

                case OP_PRINT: { Value v = pop(); std::cout << v << "\n"; break; }
                case OP_INPUT: {
                    int64_t x;
                    if (!(std::cin >> x))
                        throw std::runtime_error("Failed to read integer from stdin.");
                    push(Value::makeInt(x));
                    break;
                }

                default:
                    throw std::runtime_error("Unknown opcode: " + std::to_string(instr));
            }
        }
    }
};

} // namespace cvm
