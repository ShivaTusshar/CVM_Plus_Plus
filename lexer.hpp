#pragma once
#include "token.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <stdexcept>

namespace cvm {

class Lexer {
public:
    explicit Lexer(std::string src) : source(std::move(src)) {}

    std::vector<Token> scanTokens() {
        std::vector<Token> tokens;
        while (!isAtEnd()) {
            start = current;
            scanToken(tokens);
        }
        tokens.push_back({TokenType::EOF_TOK, "", line});
        return tokens;
    }

private:
    std::string source;
    size_t start = 0, current = 0;
    int    line = 1;

    static const std::unordered_map<std::string, TokenType>& keywords() {
        static const std::unordered_map<std::string, TokenType> kw = {
            {"let",      TokenType::LET},
            {"if",       TokenType::IF},
            {"else",     TokenType::ELSE},
            {"while",    TokenType::WHILE},
            {"for",      TokenType::FOR},
            {"break",    TokenType::BREAK},
            {"continue", TokenType::CONTINUE},
            {"fn",       TokenType::FN},
            {"return",   TokenType::RETURN},
            {"true",     TokenType::TRUE},
            {"false",    TokenType::FALSE},
            {"nil",      TokenType::NIL},
            {"print",    TokenType::PRINT},
            {"input",    TokenType::INPUT},
        };
        return kw;
    }

    bool isAtEnd() const { return current >= source.size(); }
    char advance()       { return source[current++]; }
    char peek() const    { return isAtEnd() ? '\0' : source[current]; }

    bool match(char expected) {
        if (isAtEnd() || source[current] != expected) return false;
        ++current;
        return true;
    }

    void addToken(std::vector<Token>& out, TokenType t) {
        out.push_back({t, source.substr(start, current - start), line});
    }

    void scanToken(std::vector<Token>& out) {
        char c = advance();
        switch (c) {
            case '(': addToken(out, TokenType::LPAREN); break;
            case ')': addToken(out, TokenType::RPAREN); break;
            case '{': addToken(out, TokenType::LBRACE); break;
            case '}': addToken(out, TokenType::RBRACE); break;
            case ';': addToken(out, TokenType::SEMICOLON); break;
            case ',': addToken(out, TokenType::COMMA); break;
            case '+': addToken(out, TokenType::PLUS); break;
            case '-': addToken(out, TokenType::MINUS); break;
            case '*': addToken(out, TokenType::STAR); break;
            case '/':
                if (match('/')) {
                    while (!isAtEnd() && peek() != '\n') advance();
                } else {
                    addToken(out, TokenType::SLASH);
                }
                break;
            case '<': addToken(out, TokenType::LESS); break;
            case '!': addToken(out, TokenType::BANG); break;
            case '=':
                if (match('=')) addToken(out, TokenType::EQUAL_EQUAL);
                else            addToken(out, TokenType::EQUAL);
                break;
            case '&':
                if (match('&')) addToken(out, TokenType::AMP_AMP);
                else throw std::runtime_error("[line " + std::to_string(line) +
                                              "] Unexpected '&' (did you mean '&&'?).");
                break;
            case '|':
                if (match('|')) addToken(out, TokenType::PIPE_PIPE);
                else throw std::runtime_error("[line " + std::to_string(line) +
                                              "] Unexpected '|' (did you mean '||'?).");
                break;

            case ' ': case '\r': case '\t': break;
            case '\n': ++line; break;

            default:
                if (std::isdigit(static_cast<unsigned char>(c))) {
                    number(out);
                } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                    identifier(out);
                } else {
                    throw std::runtime_error("[line " + std::to_string(line) +
                                             "] Unexpected character: '" + std::string(1, c) + "'");
                }
                break;
        }
    }

    void number(std::vector<Token>& out) {
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
        addToken(out, TokenType::NUMBER);
    }

    void identifier(std::vector<Token>& out) {
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
        std::string text = source.substr(start, current - start);
        auto it = keywords().find(text);
        addToken(out, it == keywords().end() ? TokenType::IDENTIFIER : it->second);
    }
};

} // namespace cvm
