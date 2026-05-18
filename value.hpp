#pragma once
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace cvm {

struct Function; // forward; full definition in function.hpp

enum class ValueType : uint8_t { NIL, INT, BOOL, FUNCTION };

// 16-byte tagged union covering all runtime values.
struct Value {
    ValueType type;
    union {
        int64_t          i;
        bool             b;
        const Function*  fn;
    } as;

    static Value makeNil()                       { Value v; v.type = ValueType::NIL;      v.as.i = 0; return v; }
    static Value makeInt(int64_t x)              { Value v; v.type = ValueType::INT;      v.as.i = x; return v; }
    static Value makeBool(bool x)                { Value v; v.type = ValueType::BOOL;     v.as.b = x; return v; }
    static Value makeFunction(const Function* f) { Value v; v.type = ValueType::FUNCTION; v.as.fn = f; return v; }

    bool isNil()      const { return type == ValueType::NIL;      }
    bool isInt()      const { return type == ValueType::INT;      }
    bool isBool()     const { return type == ValueType::BOOL;     }
    bool isFunction() const { return type == ValueType::FUNCTION; }

    bool equals(const Value& o) const {
        if (type != o.type) return false;
        switch (type) {
            case ValueType::NIL:      return true;
            case ValueType::INT:      return as.i  == o.as.i;
            case ValueType::BOOL:     return as.b  == o.as.b;
            case ValueType::FUNCTION: return as.fn == o.as.fn;
        }
        return false;
    }
};

// Defined in function.hpp once Function is complete.
std::ostream& operator<<(std::ostream& os, const Value& v);

} // namespace cvm
