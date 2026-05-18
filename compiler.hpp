#pragma once
#include "ast.hpp"
#include "chunk.hpp"
#include "function.hpp"
#include "opcodes.hpp"
#include "value.hpp"
#include <climits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace cvm {

// Walks the AST and emits bytecode into one Function per `fn` declaration
// (plus an implicit "<script>" function for the top-level program).
//
// State that's local to a single function compile (locals, scope depth,
// active loops) is held in a `FuncCompiler`. We push/pop these as we
// descend into nested function declarations.
class Compiler {
public:
    // Resulting top-level function. Owned here; pointer stays valid for
    // the lifetime of the Compiler.
    Function* compile(const std::vector<StmtPtr>& program) {
        beginFunction("<script>", 0);
        for (const auto& s : program) compileDecl(*s);
        emitImplicitReturn(program.empty() ? 1 : program.back()->line);
        return endFunction();
    }

    // All functions allocated during compilation. The VM holds a reference
    // so the heap memory stays alive for the entire run.
    std::vector<std::unique_ptr<Function>>& functions() { return owned; }

private:
    // ---- per-function compilation state ----
    struct Local {
        std::string name;
        int         depth;       // scope depth where this local was declared
    };
    struct LoopCtx {
        int                  localsAtStart;  // locals.size() when loop began
        int                  scopeAtStart;   // scopeDepth when loop began
        std::vector<size_t>  continueJumps;  // OP_JUMP offsets to patch at continueTarget
        std::vector<size_t>  breakJumps;     // OP_JUMP offsets to patch at loop exit
    };
    struct FuncCompiler {
        Function*               function = nullptr;
        std::vector<Local>      locals;
        int                     scopeDepth = 0;
        std::vector<LoopCtx>    loops;
        FuncCompiler*           enclosing = nullptr;
    };

    std::vector<std::unique_ptr<Function>> owned;
    FuncCompiler*                          cur = nullptr;

    // ---- function frame management ----
    void beginFunction(const std::string& name, int arity) {
        owned.emplace_back(std::make_unique<Function>());
        Function* f = owned.back().get();
        f->name = name;
        f->arity = arity;

        auto fc = new FuncCompiler();
        fc->function   = f;
        fc->scopeDepth = 0;
        fc->enclosing  = cur;
        // Slot 0 is reserved for the function itself (callable) per call ABI.
        fc->locals.push_back({"", 0});
        cur = fc;
    }

    Function* endFunction() {
        Function* f = cur->function;
        FuncCompiler* parent = cur->enclosing;
        delete cur;
        cur = parent;
        return f;
    }

    // ---- chunk helpers ----
    Chunk& chunk() { return cur->function->chunk; }

    void emitByte(uint8_t b, int line) { chunk().write(b, line); }
    void emitBytes(uint8_t a, uint8_t b, int line) { emitByte(a, line); emitByte(b, line); }

    size_t emitJump(uint8_t op, int line) {
        emitByte(op, line);
        emitByte(0xff, line);
        emitByte(0xff, line);
        return chunk().code.size() - 2;
    }

    void patchJump(size_t at) {
        size_t jump = chunk().code.size() - at - 2;
        if (jump > UINT16_MAX) throw std::runtime_error("Jump distance >65535.");
        chunk().code[at]     = static_cast<uint8_t>((jump >> 8) & 0xff);
        chunk().code[at + 1] = static_cast<uint8_t>(jump & 0xff);
    }

    void emitLoop(size_t loopStart, int line) {
        emitByte(OP_LOOP, line);
        size_t offset = chunk().code.size() - loopStart + 2;
        if (offset > UINT16_MAX) throw std::runtime_error("Loop body too large.");
        emitByte(static_cast<uint8_t>((offset >> 8) & 0xff), line);
        emitByte(static_cast<uint8_t>(offset & 0xff), line);
    }

    void emitImplicitReturn(int line) {
        emitByte(OP_NIL, line);
        emitByte(OP_RETURN, line);
    }

    // ---- name interning for the CURRENT function ----
    uint8_t identifierConstant(const std::string& name) {
        auto& names = cur->function->names;
        for (size_t i = 0; i < names.size(); ++i)
            if (names[i] == name) return static_cast<uint8_t>(i);
        if (names.size() >= 256)
            throw std::runtime_error("Too many distinct globals in one function (max 256).");
        names.push_back(name);
        return static_cast<uint8_t>(names.size() - 1);
    }

    // ---- locals ----
    void beginScope() { cur->scopeDepth++; }

    void endScope(int line) {
        while (!cur->locals.empty() && cur->locals.back().depth >= cur->scopeDepth) {
            emitByte(OP_POP, line);
            cur->locals.pop_back();
        }
        cur->scopeDepth--;
    }

    // Returns slot index, or -1 if not a local.
    int resolveLocal(const std::string& name) {
        for (int i = static_cast<int>(cur->locals.size()) - 1; i >= 0; --i)
            if (cur->locals[i].name == name) return i;
        return -1;
    }

    void addLocal(const std::string& name, int line) {
        if (cur->locals.size() >= 256)
            throw std::runtime_error("Too many locals in one function (max 256).");
        // Check for duplicate in same scope.
        for (int i = static_cast<int>(cur->locals.size()) - 1; i >= 0; --i) {
            if (cur->locals[i].depth < cur->scopeDepth) break;
            if (cur->locals[i].name == name)
                throw std::runtime_error("[line " + std::to_string(line) +
                                         "] Variable '" + name + "' already declared in this scope.");
        }
        cur->locals.push_back({name, cur->scopeDepth});
    }

    // ---- declarations ----
    void compileDecl(const Stmt& s) {
        switch (s.kind) {
            case StmtKind::FnDecl:    fnDecl(s);    break;
            case StmtKind::VarDecl:   varDecl(s);   break;
            default:                  compileStmt(s); break;
        }
    }

    void fnDecl(const Stmt& s) {
        // We compile a brand-new function. Its name becomes a global so it
        // can recurse via OP_GET_GLOBAL.
        beginFunction(s.name, static_cast<int>(s.params.size()));

        // Parameters become locals at slots 1..arity.
        beginScope();
        for (const auto& p : s.params) addLocal(p, s.line);
        // Compile the body's block statements directly (no extra scope —
        // the function's outer scope already provides one).
        if (s.body) {
            for (const auto& sub : s.body->statements) compileDecl(*sub);
        }
        emitImplicitReturn(s.line);
        // No matching endScope(): OP_RETURN cleans the stack at runtime,
        // and the per-function compiler state is about to be destroyed.

        Function* compiled = endFunction();

        // In the outer function, embed the compiled function as a constant
        // and bind it to a global named after the function.
        uint8_t kidx = chunk().addConstant(Value::makeFunction(compiled));
        emitBytes(OP_CONSTANT, kidx, s.line);

        uint8_t nidx = identifierConstant(s.name);
        emitBytes(OP_DEFINE_GLOBAL, nidx, s.line);
    }

    void varDecl(const Stmt& s) {
        compileExpr(*s.expr);                  // pushes initializer
        if (cur->scopeDepth == 0) {
            // Global
            uint8_t idx = identifierConstant(s.name);
            emitBytes(OP_DEFINE_GLOBAL, idx, s.line);
        } else {
            // Local: the initializer value already sits where the local lives.
            addLocal(s.name, s.line);
        }
    }

    // ---- statements ----
    void compileStmt(const Stmt& s) {
        switch (s.kind) {
            case StmtKind::Expression:
                compileExpr(*s.expr);
                emitByte(OP_POP, s.line);
                break;

            case StmtKind::Print:
                compileExpr(*s.expr);
                emitByte(OP_PRINT, s.line);
                break;

            case StmtKind::Block:
                beginScope();
                for (const auto& sub : s.statements) compileDecl(*sub);
                endScope(s.line);
                break;

            case StmtKind::If: {
                compileExpr(*s.expr);
                size_t thenJump = emitJump(OP_JUMP_IF_FALSE, s.line);
                emitByte(OP_POP, s.line);
                compileStmt(*s.thenBranch);
                size_t elseJump = emitJump(OP_JUMP, s.line);
                patchJump(thenJump);
                emitByte(OP_POP, s.line);
                if (s.elseBranch) compileStmt(*s.elseBranch);
                patchJump(elseJump);
                break;
            }

            case StmtKind::While:    whileLoop(s);    break;
            case StmtKind::For:      forLoop(s);      break;
            case StmtKind::Break:    breakStmt(s);    break;
            case StmtKind::Continue: continueStmt(s); break;
            case StmtKind::Return:   returnStmt(s);   break;

            // Declarations should never land here, but be defensive.
            case StmtKind::VarDecl:  varDecl(s); break;
            case StmtKind::FnDecl:   fnDecl(s);  break;
        }
    }

    // ---- loops ----
    void pushLoop() {
        LoopCtx ctx;
        ctx.localsAtStart = static_cast<int>(cur->locals.size());
        ctx.scopeAtStart  = cur->scopeDepth;
        cur->loops.push_back(std::move(ctx));
    }
    LoopCtx& topLoop() {
        if (cur->loops.empty())
            throw std::runtime_error("'break'/'continue' outside any loop.");
        return cur->loops.back();
    }
    void popLoopAndPatchBreaks() {
        LoopCtx ctx = std::move(cur->loops.back());
        cur->loops.pop_back();
        for (size_t off : ctx.breakJumps) patchJump(off);
    }

    void whileLoop(const Stmt& s) {
        size_t loopStart = chunk().code.size();
        compileExpr(*s.expr);
        size_t exitJump = emitJump(OP_JUMP_IF_FALSE, s.line);
        emitByte(OP_POP, s.line);

        pushLoop();
        compileStmt(*s.body);

        // continue jumps target THIS spot (just before the loop-back).
        for (size_t off : topLoop().continueJumps) patchJump(off);
        emitLoop(loopStart, s.line);

        patchJump(exitJump);
        emitByte(OP_POP, s.line);
        popLoopAndPatchBreaks();
    }

    void forLoop(const Stmt& s) {
        // The init may declare a fresh local (e.g. `for (let i = 0; ...)`)
        // — that needs its own scope so the local dies with the loop.
        beginScope();
        if (s.forInit) compileDecl(*s.forInit);

        size_t loopStart = chunk().code.size();

        // condition (or implicit true)
        size_t exitJump = SIZE_MAX;
        if (s.expr) {
            compileExpr(*s.expr);
            exitJump = emitJump(OP_JUMP_IF_FALSE, s.line);
            emitByte(OP_POP, s.line);
        }

        pushLoop();
        compileStmt(*s.body);

        // continue target = right before the step expression
        for (size_t off : topLoop().continueJumps) patchJump(off);
        if (s.forStep) {
            compileExpr(*s.forStep);
            emitByte(OP_POP, s.line);
        }
        emitLoop(loopStart, s.line);

        if (exitJump != SIZE_MAX) {
            patchJump(exitJump);
            emitByte(OP_POP, s.line);
        }
        popLoopAndPatchBreaks();
        endScope(s.line);
    }

    // Pop any locals declared inside the current loop before jumping out/around.
    void popLoopLocals(int line) {
        int target = topLoop().localsAtStart;
        for (int i = static_cast<int>(cur->locals.size()); i > target; --i)
            emitByte(OP_POP, line);
    }

    void breakStmt(const Stmt& s) {
        if (cur->loops.empty())
            throw std::runtime_error("[line " + std::to_string(s.line) + "] 'break' outside any loop.");
        popLoopLocals(s.line);
        size_t j = emitJump(OP_JUMP, s.line);
        topLoop().breakJumps.push_back(j);
    }

    void continueStmt(const Stmt& s) {
        if (cur->loops.empty())
            throw std::runtime_error("[line " + std::to_string(s.line) + "] 'continue' outside any loop.");
        popLoopLocals(s.line);
        size_t j = emitJump(OP_JUMP, s.line);
        topLoop().continueJumps.push_back(j);
    }

    void returnStmt(const Stmt& s) {
        if (cur->enclosing == nullptr)
            throw std::runtime_error("[line " + std::to_string(s.line) + "] 'return' outside any function.");
        if (s.expr) compileExpr(*s.expr);
        else        emitByte(OP_NIL, s.line);
        emitByte(OP_RETURN, s.line);
    }

    // ---- expressions ----
    void compileExpr(const Expr& e) {
        switch (e.kind) {
            case ExprKind::NumberLit: {
                uint8_t idx = chunk().addConstant(Value::makeInt(e.intValue));
                emitBytes(OP_CONSTANT, idx, e.line);
                break;
            }
            case ExprKind::BoolLit:
                emitByte(e.boolValue ? OP_TRUE : OP_FALSE, e.line);
                break;
            case ExprKind::NilLit:
                emitByte(OP_NIL, e.line);
                break;
            case ExprKind::Input:
                emitByte(OP_INPUT, e.line);
                break;

            case ExprKind::Variable: {
                int slot = resolveLocal(e.name);
                if (slot >= 0) emitBytes(OP_GET_LOCAL, static_cast<uint8_t>(slot), e.line);
                else {
                    uint8_t idx = identifierConstant(e.name);
                    emitBytes(OP_GET_GLOBAL, idx, e.line);
                }
                break;
            }

            case ExprKind::Assign: {
                compileExpr(*e.value);
                int slot = resolveLocal(e.name);
                if (slot >= 0) emitBytes(OP_SET_LOCAL, static_cast<uint8_t>(slot), e.line);
                else {
                    uint8_t idx = identifierConstant(e.name);
                    emitBytes(OP_SET_GLOBAL, idx, e.line);
                }
                break;
            }

            case ExprKind::Unary:
                compileExpr(*e.right);
                if (e.op == TokenType::MINUS)     emitByte(OP_NEGATE, e.line);
                else if (e.op == TokenType::BANG) emitByte(OP_NOT,    e.line);
                else throw std::runtime_error("Unknown unary operator.");
                break;

            case ExprKind::Binary:
                compileExpr(*e.left);
                compileExpr(*e.right);
                switch (e.op) {
                    case TokenType::PLUS:        emitByte(OP_ADD,   e.line); break;
                    case TokenType::MINUS:       emitByte(OP_SUB,   e.line); break;
                    case TokenType::STAR:        emitByte(OP_MUL,   e.line); break;
                    case TokenType::SLASH:       emitByte(OP_DIV,   e.line); break;
                    case TokenType::EQUAL_EQUAL: emitByte(OP_EQUAL, e.line); break;
                    case TokenType::LESS:        emitByte(OP_LESS,  e.line); break;
                    default: throw std::runtime_error("Unknown binary operator.");
                }
                break;

            case ExprKind::Logical: {
                // && : if LHS false, leave LHS on stack and skip RHS.
                // || : if LHS true,  leave LHS on stack and skip RHS.
                compileExpr(*e.left);
                uint8_t op = (e.op == TokenType::AMP_AMP) ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE;
                size_t skipRhs = emitJump(op, e.line);
                emitByte(OP_POP, e.line);     // discard LHS, take RHS
                compileExpr(*e.right);
                patchJump(skipRhs);
                break;
            }

            case ExprKind::Call: {
                compileExpr(*e.callee);
                for (const auto& a : e.args) compileExpr(*a);
                if (e.args.size() > 255)
                    throw std::runtime_error("Too many arguments (max 255).");
                emitBytes(OP_CALL, static_cast<uint8_t>(e.args.size()), e.line);
                break;
            }
        }
    }
};

} // namespace cvm
