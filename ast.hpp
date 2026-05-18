#pragma once
#include "token.hpp"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace cvm {

struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ----- Expression nodes -----
enum class ExprKind {
    NumberLit, BoolLit, NilLit, Variable, Assign,
    Binary, Unary, Logical, Call, Input
};

struct Expr {
    ExprKind  kind;
    int       line = 0;

    // NumberLit
    int64_t   intValue = 0;
    // BoolLit
    bool      boolValue = false;
    // Variable / Assign / Call(callee-name if simple)
    std::string name;
    // Assign
    ExprPtr   value;
    // Binary / Unary / Logical
    TokenType op = TokenType::ERROR;
    ExprPtr   left;
    ExprPtr   right;
    // Call
    ExprPtr               callee;
    std::vector<ExprPtr>  args;
};

// ----- Statement nodes -----
enum class StmtKind {
    Expression, Print, VarDecl, Block,
    If, While, For,
    Break, Continue,
    FnDecl, Return
};

struct Stmt {
    StmtKind kind;
    int      line = 0;

    // Expression / Print / VarDecl / If(cond) / While(cond) / Return(value) / For(cond)
    ExprPtr  expr;

    // VarDecl / FnDecl
    std::string name;

    // Block
    std::vector<StmtPtr> statements;

    // If
    StmtPtr  thenBranch;
    StmtPtr  elseBranch;

    // While / For
    StmtPtr  body;

    // For (init may be VarDecl or Expression or null; step is an Expr or null)
    StmtPtr  forInit;
    ExprPtr  forStep;

    // FnDecl
    std::vector<std::string> params;
};

// ----- Factory helpers -----
inline ExprPtr makeNumber(int64_t v, int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::NumberLit; e->intValue = v; e->line = line; return e;
}
inline ExprPtr makeBool(bool v, int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::BoolLit; e->boolValue = v; e->line = line; return e;
}
inline ExprPtr makeNil(int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::NilLit; e->line = line; return e;
}
inline ExprPtr makeVariable(const std::string& n, int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::Variable; e->name = n; e->line = line; return e;
}
inline ExprPtr makeAssign(const std::string& n, ExprPtr val, int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::Assign; e->name = n; e->value = std::move(val); e->line = line; return e;
}
inline ExprPtr makeBinary(TokenType op, ExprPtr l, ExprPtr r, int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::Binary; e->op = op;
    e->left = std::move(l); e->right = std::move(r); e->line = line; return e;
}
inline ExprPtr makeLogical(TokenType op, ExprPtr l, ExprPtr r, int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::Logical; e->op = op;
    e->left = std::move(l); e->right = std::move(r); e->line = line; return e;
}
inline ExprPtr makeUnary(TokenType op, ExprPtr r, int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::Unary; e->op = op; e->right = std::move(r); e->line = line; return e;
}
inline ExprPtr makeInput(int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::Input; e->line = line; return e;
}
inline ExprPtr makeCall(ExprPtr callee, std::vector<ExprPtr> args, int line) {
    auto e = std::make_unique<Expr>(); e->kind = ExprKind::Call;
    e->callee = std::move(callee); e->args = std::move(args); e->line = line; return e;
}

} // namespace cvm
