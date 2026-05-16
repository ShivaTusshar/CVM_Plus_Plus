#pragma once
#include "opcodes.hpp"
#include "value.hpp"
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace cvm {

// A Chunk is a compiled unit: raw bytecode stream + a constant pool.
// Using std::vector<uint8_t> gives us cache-friendly, RISC-pipeline-style
// linear instruction fetching in the VM's dispatch loop.
struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value>   constants;
    std::vector<int>     lines; // parallel to code[] for error reporting

    void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    // Returns index into constant pool.
    uint8_t addConstant(const Value& v) {
        constants.push_back(v);
        if (constants.size() > 255)
            throw std::runtime_error("Too many constants in one chunk (max 256).");
        return static_cast<uint8_t>(constants.size() - 1);
    }
};

} // namespace cvm
