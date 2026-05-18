#pragma once
#include <string>

namespace cvm {

enum class TokenType {
    // Literals
    NUMBER, IDENTIFIER, TRUE, FALSE, NIL,

    // Single-char operators / punctuation
    PLUS, MINUS, STAR, SLASH,
    LPAREN, RPAREN, LBRACE, RBRACE,
    SEMICOLON, COMMA,

    // One- or two-char operators
    EQUAL,            // =
    EQUAL_EQUAL,      // ==
    LESS,             // <
    BANG,             // !
    AMP_AMP,          // &&
    PIPE_PIPE,        // ||

    // Keywords
    LET, IF, ELSE, WHILE, FOR,
    BREAK, CONTINUE,
    FN, RETURN,
    PRINT, INPUT,

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
        case TokenType::NIL:         return "NIL";
        case TokenType::PLUS:        return "PLUS";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::STAR:        return "STAR";
        case TokenType::SLASH:       return "SLASH";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::LBRACE:      return "LBRACE";
        case TokenType::RBRACE:      return "RBRACE";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::COMMA:       return "COMMA";
        case TokenType::EQUAL:       return "EQUAL";
        case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::LESS:        return "LESS";
        case TokenType::BANG:        return "BANG";
        case TokenType::AMP_AMP:     return "AMP_AMP";
        case TokenType::PIPE_PIPE:   return "PIPE_PIPE";
        case TokenType::LET:         return "LET";
        case TokenType::IF:          return "IF";
        case TokenType::ELSE:        return "ELSE";
        case TokenType::WHILE:       return "WHILE";
        case TokenType::FOR:         return "FOR";
        case TokenType::BREAK:       return "BREAK";
        case TokenType::CONTINUE:    return "CONTINUE";
        case TokenType::FN:          return "FN";
        case TokenType::RETURN:      return "RETURN";
        case TokenType::PRINT:       return "PRINT";
        case TokenType::INPUT:       return "INPUT";
        case TokenType::EOF_TOK:     return "EOF";
        case TokenType::ERROR:       return "ERROR";
    }
    return "?";
}

} // namespace cvm
