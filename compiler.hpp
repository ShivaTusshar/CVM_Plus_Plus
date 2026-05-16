#pragma once
#include "ast.hpp"
#include "chunk.hpp"
#include "opcodes.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <climits>

namespace cvm {

// Walks the AST once and emits a flat std::vector<uint8_t> of bytecode.
// Globals are resolved by name at runtime; the name string is interned in
// the Compiler's `names` table and the bytecode references it by a single-
// byte index.
class Compiler {
public:
    // After compile(), `names` is populated and must be passed to the VM.
    std::vector<std::string> names;

    Chunk compile(const std::vector<StmtPtr>& program) {
        Chunk c;
        for (const auto& s : program) compileStmt(*s, c);
        c.write(OP_HALT, program.empty() ? 1 : program.back()->line);
        return c;
    }

private:
    // ---- emitters ----
    void emitByte(Chunk& c, uint8_t b, int line) { c.write(b, line); }

    // Emit a jump opcode with a 16-bit placeholder operand. Returns the
    // offset of the placeholder so it can be patched later.
    size_t emitJump(Chunk& c, uint8_t op, int line) {
        emitByte(c, op, line);
        emitByte(c, 0xff, line);
        emitByte(c, 0xff, line);
        return c.code.size() - 2;
    }

    // Patch a previously emitted jump to point at the current ip.
    void patchJump(Chunk& c, size_t offset) {
        size_t jump = c.code.size() - offset - 2;
        if (jump > UINT16_MAX) throw std::runtime_error("Jump too large (>65535 bytes).");
        c.code[offset]     = (jump >> 8) & 0xff;
        c.code[offset + 1] = jump & 0xff;
    }

    // Emit a backward jump (loop) to the given start position.
    void emitLoop(Chunk& c, size_t loopStart, int line) {
        emitByte(c, OP_LOOP, line);
        size_t offset = c.code.size() - loopStart + 2;
        if (offset > UINT16_MAX) throw std::runtime_error("Loop body too large.");
        emitByte(c, (offset >> 8) & 0xff, line);
        emitByte(c, offset & 0xff, line);
    }

    uint8_t identifierConstant(const std::string& name) {
        for (size_t i = 0; i < names.size(); ++i)
            if (names[i] == name) return static_cast<uint8_t>(i);
        if (names.size() >= 256) throw std::runtime_error("Too many distinct globals (max 256).");
        names.push_back(name);
        return static_cast<uint8_t>(names.size() - 1);
    }

    // ---- statements ----
    void compileStmt(const Stmt& s, Chunk& c) {
        switch (s.kind) {
            case StmtKind::Expression:
                compileExpr(*s.expr, c);
                emitByte(c, OP_POP, s.line);                 // discard result
                break;

            case StmtKind::Print:
                compileExpr(*s.expr, c);
                emitByte(c, OP_PRINT, s.line);
                break;

            case StmtKind::VarDecl: {
                compileExpr(*s.expr, c);
                uint8_t idx = identifierConstant(s.name);
                emitByte(c, OP_DEFINE_GLOBAL, s.line);
                emitByte(c, idx,              s.line);
                break;
            }

            case StmtKind::Block:
                for (const auto& sub : s.statements) compileStmt(*sub, c);
                break;

            case StmtKind::If: {
                compileExpr(*s.expr, c);
                size_t thenJump = emitJump(c, OP_JUMP_IF_FALSE, s.line);
                emitByte(c, OP_POP, s.line);                 // pop condition (true branch)
                compileStmt(*s.thenBranch, c);

                size_t elseJump = emitJump(c, OP_JUMP, s.line);
                patchJump(c, thenJump);
                emitByte(c, OP_POP, s.line);                 // pop condition (false branch)

                if (s.elseBranch) compileStmt(*s.elseBranch, c);
                patchJump(c, elseJump);
                break;
            }

            case StmtKind::While: {
                size_t loopStart = c.code.size();
                compileExpr(*s.expr, c);
                size_t exitJump = emitJump(c, OP_JUMP_IF_FALSE, s.line);
                emitByte(c, OP_POP, s.line);                 // pop condition (true)
                compileStmt(*s.body, c);
                emitLoop(c, loopStart, s.line);
                patchJump(c, exitJump);
                emitByte(c, OP_POP, s.line);                 // pop condition (false)
                break;
            }
        }
    }

    // ---- expressions ----
    void compileExpr(const Expr& e, Chunk& c) {
        switch (e.kind) {
            case ExprKind::NumberLit: {
                uint8_t idx = c.addConstant(Value::makeInt(e.intValue));
                emitByte(c, OP_CONSTANT, e.line);
                emitByte(c, idx,         e.line);
                break;
            }
            case ExprKind::BoolLit:
                emitByte(c, e.boolValue ? OP_TRUE : OP_FALSE, e.line);
                break;

            case ExprKind::Input:
                emitByte(c, OP_INPUT, e.line);
                break;

            case ExprKind::Variable: {
                uint8_t idx = identifierConstant(e.name);
                emitByte(c, OP_GET_GLOBAL, e.line);
                emitByte(c, idx,           e.line);
                break;
            }

            case ExprKind::Assign: {
                compileExpr(*e.value, c);
                uint8_t idx = identifierConstant(e.name);
                emitByte(c, OP_SET_GLOBAL, e.line);
                emitByte(c, idx,           e.line);
                break;
            }

            case ExprKind::Unary:
                compileExpr(*e.right, c);
                if (e.op == TokenType::MINUS)     emitByte(c, OP_NEGATE, e.line);
                else if (e.op == TokenType::BANG) emitByte(c, OP_NOT,    e.line);
                else throw std::runtime_error("Unknown unary operator.");
                break;

            case ExprKind::Binary:
                compileExpr(*e.left,  c);
                compileExpr(*e.right, c);
                switch (e.op) {
                    case TokenType::PLUS:        emitByte(c, OP_ADD,   e.line); break;
                    case TokenType::MINUS:       emitByte(c, OP_SUB,   e.line); break;
                    case TokenType::STAR:        emitByte(c, OP_MUL,   e.line); break;
                    case TokenType::SLASH:       emitByte(c, OP_DIV,   e.line); break;
                    case TokenType::EQUAL_EQUAL: emitByte(c, OP_EQUAL, e.line); break;
                    case TokenType::LESS:        emitByte(c, OP_LESS,  e.line); break;
                    default: throw std::runtime_error("Unknown binary operator.");
                }
                break;
        }
    }
};

} // namespace cvm
