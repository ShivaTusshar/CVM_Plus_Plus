#pragma once
#include "chunk.hpp"     // brings in value.hpp
#include <string>
#include <vector>
#include <ostream>

namespace cvm {

// A compiled function: name, arity, its own bytecode chunk, and its own
// pool of global-identifier strings (referenced from this fn's bytecode
// by 1-byte indices).
struct Function {
    std::string              name;
    int                      arity = 0;
    Chunk                    chunk;
    std::vector<std::string> names;
};

inline std::ostream& operator<<(std::ostream& os, const Function& f) {
    if (f.name.empty() || f.name == "<script>") os << "<script>";
    else                                        os << "<fn " << f.name << ">";
    return os;
}

// Defined here (now that Function is complete) but declared in value.hpp.
inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    switch (v.type) {
        case ValueType::NIL:      os << "nil"; break;
        case ValueType::INT:      os << v.as.i; break;
        case ValueType::BOOL:     os << (v.as.b ? "true" : "false"); break;
        case ValueType::FUNCTION: if (v.as.fn) os << *v.as.fn; else os << "<fn?>"; break;
    }
    return os;
}

} // namespace cvm
