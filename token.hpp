#pragma once
#include <string>
#include <ostream>

namespace cvm {

enum class TokenType {
    // Literals
    NUMBER, IDENTIFIER, TRUE, FALSE,

    // Single-char operators / punctuation
    PLUS, MINUS, STAR, SLASH,
    LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON,

    // One- or two-char operators
    EQUAL,        // =
    EQUAL_EQUAL,  // ==
    LESS,         // <
    BANG,         // !  (reserved; not in grammar but harmless)

    // Keywords
    LET, IF, ELSE, WHILE, PRINT, INPUT,

    // Bookkeeping
    EOF_TOK, ERROR
};

struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;
};

inline const char* tokenName(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::TRUE:        return "TRUE";
        case TokenType::FALSE:       return "FALSE";
        case TokenType::PLUS:        return "PLUS";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::STAR:        return "STAR";
        case TokenType::SLASH:       return "SLASH";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::LBRACE:      return "LBRACE";
        case TokenType::RBRACE:      return "RBRACE";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::EQUAL:       return "EQUAL";
        case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::LESS:        return "LESS";
        case TokenType::BANG:        return "BANG";
        case TokenType::LET:         return "LET";
        case TokenType::IF:          return "IF";
        case TokenType::ELSE:        return "ELSE";
        case TokenType::WHILE:       return "WHILE";
        case TokenType::PRINT:       return "PRINT";
        case TokenType::INPUT:       return "INPUT";
        case TokenType::EOF_TOK:     return "EOF";
        case TokenType::ERROR:       return "ERROR";
    }
    return "?";
}

} // namespace cvm
