#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <vector>
#include <stdexcept>
#include <initializer_list>

namespace cvm {

// Recursive-descent parser. Grammar (lowest to highest precedence):
//
//   program     -> statement* EOF
//   statement   -> letDecl | printStmt | ifStmt | whileStmt | block | exprStmt
//   letDecl     -> "let" IDENT "=" expression ";"
//   printStmt   -> "print" expression ";"
//   ifStmt      -> "if" "(" expression ")" statement ("else" statement)?
//   whileStmt   -> "while" "(" expression ")" statement
//   block       -> "{" statement* "}"
//   exprStmt    -> expression ";"
//
//   expression  -> assignment
//   assignment  -> IDENT "=" assignment | equality
//   equality    -> comparison ( "==" comparison )*
//   comparison  -> term ( "<" term )*
//   term        -> factor ( ("+"|"-") factor )*
//   factor      -> unary ( ("*"|"/") unary )*
//   unary       -> ("-"|"!") unary | primary
//   primary     -> NUMBER | "true" | "false" | "input" | IDENT | "(" expression ")"
class Parser {
public:
    explicit Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

    std::vector<StmtPtr> parse() {
        std::vector<StmtPtr> stmts;
        while (!isAtEnd()) stmts.push_back(statement());
        return stmts;
    }

private:
    std::vector<Token> tokens;
    size_t pos = 0;

    // --- helpers ----
    const Token& peek() const     { return tokens[pos]; }
    const Token& previous() const { return tokens[pos - 1]; }
    bool isAtEnd() const          { return peek().type == TokenType::EOF_TOK; }
    bool check(TokenType t) const { return !isAtEnd() && peek().type == t; }
    const Token& advance()        { if (!isAtEnd()) ++pos; return previous(); }
    bool match(std::initializer_list<TokenType> ts) {
        for (auto t : ts) if (check(t)) { advance(); return true; }
        return false;
    }
    const Token& consume(TokenType t, const std::string& msg) {
        if (check(t)) return advance();
        throw std::runtime_error("[line " + std::to_string(peek().line) + "] Parse error: " +
                                 msg + " (got '" + peek().lexeme + "')");
    }

    // --- statements ----
    StmtPtr statement() {
        if (match({TokenType::LET}))    return letDecl();
        if (match({TokenType::PRINT}))  return printStmt();
        if (match({TokenType::IF}))     return ifStmt();
        if (match({TokenType::WHILE}))  return whileStmt();
        if (match({TokenType::LBRACE})) return block();
        return exprStmt();
    }

    StmtPtr letDecl() {
        int ln = previous().line;
        const Token& name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'.");
        consume(TokenType::EQUAL, "Expected '=' after variable name.");
        ExprPtr init = expression();
        consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::VarDecl; s->name = name.lexeme; s->expr = std::move(init); s->line = ln;
        return s;
    }

    StmtPtr printStmt() {
        int ln = previous().line;
        ExprPtr v = expression();
        consume(TokenType::SEMICOLON, "Expected ';' after print value.");
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Print; s->expr = std::move(v); s->line = ln;
        return s;
    }

    StmtPtr ifStmt() {
        int ln = previous().line;
        consume(TokenType::LPAREN, "Expected '(' after 'if'.");
        ExprPtr cond = expression();
        consume(TokenType::RPAREN, "Expected ')' after if condition.");
        StmtPtr thenB = statement();
        StmtPtr elseB = nullptr;
        if (match({TokenType::ELSE})) elseB = statement();
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::If; s->expr = std::move(cond);
        s->thenBranch = std::move(thenB); s->elseBranch = std::move(elseB); s->line = ln;
        return s;
    }

    StmtPtr whileStmt() {
        int ln = previous().line;
        consume(TokenType::LPAREN, "Expected '(' after 'while'.");
        ExprPtr cond = expression();
        consume(TokenType::RPAREN, "Expected ')' after while condition.");
        StmtPtr body = statement();
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::While; s->expr = std::move(cond);
        s->body = std::move(body); s->line = ln;
        return s;
    }

    StmtPtr block() {
        int ln = previous().line;
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Block; s->line = ln;
        while (!check(TokenType::RBRACE) && !isAtEnd())
            s->statements.push_back(statement());
        consume(TokenType::RBRACE, "Expected '}' to close block.");
        return s;
    }

    StmtPtr exprStmt() {
        int ln = peek().line;
        ExprPtr e = expression();
        consume(TokenType::SEMICOLON, "Expected ';' after expression.");
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Expression; s->expr = std::move(e); s->line = ln;
        return s;
    }

    // --- expressions ----
    ExprPtr expression() { return assignment(); }

    ExprPtr assignment() {
        ExprPtr expr = equality();
        if (match({TokenType::EQUAL})) {
            int ln = previous().line;
            ExprPtr value = assignment();
            if (expr->kind != ExprKind::Variable)
                throw std::runtime_error("[line " + std::to_string(ln) + "] Invalid assignment target.");
            return makeAssign(expr->name, std::move(value), ln);
        }
        return expr;
    }

    ExprPtr equality() {
        ExprPtr expr = comparison();
        while (match({TokenType::EQUAL_EQUAL})) {
            TokenType op = previous().type;
            int ln = previous().line;
            ExprPtr right = comparison();
            expr = makeBinary(op, std::move(expr), std::move(right), ln);
        }
        return expr;
    }

    ExprPtr comparison() {
        ExprPtr expr = term();
        while (match({TokenType::LESS})) {
            TokenType op = previous().type;
            int ln = previous().line;
            ExprPtr right = term();
            expr = makeBinary(op, std::move(expr), std::move(right), ln);
        }
        return expr;
    }

    ExprPtr term() {
        ExprPtr expr = factor();
        while (match({TokenType::PLUS, TokenType::MINUS})) {
            TokenType op = previous().type;
            int ln = previous().line;
            ExprPtr right = factor();
            expr = makeBinary(op, std::move(expr), std::move(right), ln);
        }
        return expr;
    }

    ExprPtr factor() {
        ExprPtr expr = unary();
        while (match({TokenType::STAR, TokenType::SLASH})) {
            TokenType op = previous().type;
            int ln = previous().line;
            ExprPtr right = unary();
            expr = makeBinary(op, std::move(expr), std::move(right), ln);
        }
        return expr;
    }

    ExprPtr unary() {
        if (match({TokenType::MINUS, TokenType::BANG})) {
            TokenType op = previous().type;
            int ln = previous().line;
            ExprPtr right = unary();
            return makeUnary(op, std::move(right), ln);
        }
        return primary();
    }

    ExprPtr primary() {
        if (match({TokenType::NUMBER})) {
            int64_t v = std::stoll(previous().lexeme);
            return makeNumber(v, previous().line);
        }
        if (match({TokenType::TRUE}))  return makeBool(true,  previous().line);
        if (match({TokenType::FALSE})) return makeBool(false, previous().line);
        if (match({TokenType::INPUT})) return makeInput(previous().line);
        if (match({TokenType::IDENTIFIER}))
            return makeVariable(previous().lexeme, previous().line);
        if (match({TokenType::LPAREN})) {
            ExprPtr e = expression();
            consume(TokenType::RPAREN, "Expected ')' after expression.");
            return e;
        }
        throw std::runtime_error("[line " + std::to_string(peek().line) +
                                 "] Expected expression, got '" + peek().lexeme + "'.");
    }
};

} // namespace cvm
