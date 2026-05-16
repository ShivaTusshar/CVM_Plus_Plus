#pragma once
#include "chunk.hpp"
#include "opcodes.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace cvm {

class Disassembler {
public:
    static void disassemble(const Chunk& c,
                            const std::vector<std::string>& names,
                            const std::string& title)
    {
        std::printf("== %s ==\n", title.c_str());
        for (size_t off = 0; off < c.code.size(); )
            off = instr(c, names, off);
    }

private:
    static size_t simple(const char* name, size_t off) {
        std::printf("%-20s\n", name);
        return off + 1;
    }
    static size_t constInstr(const char* name, const Chunk& c, size_t off) {
        uint8_t idx = c.code[off + 1];
        std::printf("%-20s %4d  '", name, idx);
        std::cout << c.constants[idx] << "'\n";
        return off + 2;
    }
    static size_t nameInstr(const char* name, const Chunk& c,
                            const std::vector<std::string>& names, size_t off) {
        uint8_t idx = c.code[off + 1];
        std::printf("%-20s %4d  '%s'\n", name, idx,
                    idx < names.size() ? names[idx].c_str() : "?");
        return off + 2;
    }
    static size_t jumpInstr(const char* name, int sign, const Chunk& c, size_t off) {
        uint16_t jmp = (uint16_t)((c.code[off + 1] << 8) | c.code[off + 2]);
        std::printf("%-20s %4zu -> %zu\n", name, off, off + 3 + sign * jmp);
        return off + 3;
    }
    static size_t instr(const Chunk& c, const std::vector<std::string>& names, size_t off) {
        std::printf("%04zu  ", off);
        if (off > 0 && c.lines[off] == c.lines[off - 1]) std::printf("   |  ");
        else                                             std::printf("%4d  ", c.lines[off]);
        uint8_t op = c.code[off];
        switch (op) {
            case OP_CONSTANT:      return constInstr("OP_CONSTANT",     c, off);
            case OP_TRUE:          return simple("OP_TRUE",     off);
            case OP_FALSE:         return simple("OP_FALSE",    off);
            case OP_POP:           return simple("OP_POP",      off);
            case OP_DEFINE_GLOBAL: return nameInstr("OP_DEFINE_GLOBAL", c, names, off);
            case OP_GET_GLOBAL:    return nameInstr("OP_GET_GLOBAL",    c, names, off);
            case OP_SET_GLOBAL:    return nameInstr("OP_SET_GLOBAL",    c, names, off);
            case OP_ADD:           return simple("OP_ADD",      off);
            case OP_SUB:           return simple("OP_SUB",      off);
            case OP_MUL:           return simple("OP_MUL",      off);
            case OP_DIV:           return simple("OP_DIV",      off);
            case OP_NEGATE:        return simple("OP_NEGATE",   off);
            case OP_EQUAL:         return simple("OP_EQUAL",    off);
            case OP_LESS:          return simple("OP_LESS",     off);
            case OP_NOT:           return simple("OP_NOT",      off);
            case OP_JUMP:          return jumpInstr("OP_JUMP",          1, c, off);
            case OP_JUMP_IF_FALSE: return jumpInstr("OP_JUMP_IF_FALSE", 1, c, off);
            case OP_LOOP:          return jumpInstr("OP_LOOP",         -1, c, off);
            case OP_PRINT:         return simple("OP_PRINT",    off);
            case OP_INPUT:         return simple("OP_INPUT",    off);
            case OP_HALT:          return simple("OP_HALT",     off);
            default: std::printf("Unknown opcode %d\n", op); return off + 1;
        }
    }
};

} // namespace cvm
