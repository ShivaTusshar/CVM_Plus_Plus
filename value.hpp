#pragma once
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace cvm {

enum class ValueType : uint8_t {
    INT,
    BOOL
};

// Compact tagged union. Fits in 16 bytes on most platforms.
struct Value {
    ValueType type;
    union {
        int64_t i;
        bool    b;
    } as;

    static Value makeInt(int64_t v)  { Value x; x.type = ValueType::INT;  x.as.i = v; return x; }
    static Value makeBool(bool v)    { Value x; x.type = ValueType::BOOL; x.as.b = v; return x; }

    bool isInt()  const { return type == ValueType::INT;  }
    bool isBool() const { return type == ValueType::BOOL; }

    int64_t asInt()  const {
        if (type != ValueType::INT)  throw std::runtime_error("Runtime type error: expected Int");
        return as.i;
    }
    bool asBool() const {
        if (type != ValueType::BOOL) throw std::runtime_error("Runtime type error: expected Bool");
        return as.b;
    }

    bool equals(const Value& o) const {
        if (type != o.type) return false;
        if (type == ValueType::INT)  return as.i == o.as.i;
        return as.b == o.as.b;
    }

    friend std::ostream& operator<<(std::ostream& os, const Value& v) {
        if (v.type == ValueType::INT)  os << v.as.i;
        else                           os << (v.as.b ? "true" : "false");
        return os;
    }
};

} // namespace cvm
