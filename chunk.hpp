#pragma once
#include "opcodes.hpp"
#include "value.hpp"
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace cvm {

// Compiled unit: raw bytecode + per-function constant pool + line table.
struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value>   constants;
    std::vector<int>     lines;    // parallel to code[]

    void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    uint8_t addConstant(const Value& v) {
        constants.push_back(v);
        if (constants.size() > 255)
            throw std::runtime_error("Too many constants in one function (max 256).");
        return static_cast<uint8_t>(constants.size() - 1);
    }
};

} // namespace cvm
