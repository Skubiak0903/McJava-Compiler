// tokenization.hpp
#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "./registries/SimplifiedCommandRegistry.hpp"

enum class TokenType {
    //litelals
    INT_LIT,
    FLOAT_LIT,
    STRING_LIT,

    // other
    SEMI_COLON,
    OPEN_PAREN,
    CLOSE_PAREN,
    EQUALS,

    // dynamic tokens
    IDENT,
    CMD_KEY,

    // special
    NEW_LINE,
    END_OF_FILE,
};

struct Token {
    TokenType type;
    std::optional<std::string> value {};
    size_t line;
    size_t col;
};

std::string tokenTypeToString(TokenType type) {
    switch(type) {
        //litelals
        case TokenType::INT_LIT : return "Int";
        case TokenType::FLOAT_LIT : return "Float"; 
        case TokenType::STRING_LIT : return "String";
        
        // other
        case TokenType::SEMI_COLON : return ";";
        case TokenType::OPEN_PAREN : return "(";
        case TokenType::CLOSE_PAREN : return ")";

        // dynamic tokens
        case TokenType::IDENT : return "IDENTIFIER";
        case TokenType::CMD_KEY : return "COMMAND_KEY";
        
        // special
        case TokenType::NEW_LINE: return "NEW_LINE";
        case TokenType::END_OF_FILE : return "END_OF_FILE";
        default: return "UNKNOWN";
    }
}

class Tokenizer {
public:
    inline explicit Tokenizer(const std::string src, SimplifiedCommandRegistry& registry)
        : m_src(std::move(src)), m_reg(registry) {}

    std::vector<Token> tokenize() 
    {   
        std::vector<Token> tokens;
        std::string buf;
        while(peek().has_value()) {
            if (std::isalpha(peek().value())) {
                buf.push_back(consume());
                while (peek().has_value() &&
                    (std::isalnum(peek().value()) || peek().value() == '_' || peek().value() == '-')) {
                    buf.push_back(consume());
                }
                if (m_reg.isValid(buf)) {
                    tokens.push_back({.type = TokenType::CMD_KEY, .value = buf, .line = line, .col = col });
                } else {
                    tokens.push_back({.type = TokenType::IDENT, .value = buf, .line = line, .col = col });
                } 
                buf.clear();
            }
            else if (std::isdigit(peek().value()) ||
                    (peek().value() == '-' && (std::isdigit(peek(1).value_or('0')) || peek(1).value_or('\0') == '.')) ||
                    peek().value() == '.')
            {
                std::string buf;       // pierwsza liczba
                bool isFloat = false;

                // minus opcjonalny
                if (peek().value() == '-') buf.push_back(consume());

                // część całkowita
                while (peek().has_value() && std::isdigit(peek().value()))
                    buf.push_back(consume());

                // sprawdzamy czy to float
                if (peek().has_value() && peek().value() == '.') {
                    // float
                    isFloat = true;
                    buf.push_back(consume()); // kropka
                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buf.push_back(consume());
                    }
                }

                // tworzymy token
                if (isFloat) {
                    tokens.push_back({ .type = TokenType::FLOAT_LIT,
                                    .value = buf,
                                    .line = line, .col = col });
                } else {
                    tokens.push_back({ .type = TokenType::INT_LIT,
                                    .value = buf,
                                    .line = line, .col = col });
                }
            }
            else if (peek().value() == '"' || peek().value() == '\'') {
                char quote = consume(); // " lub '
                while (peek().has_value() && peek().value() != quote) {
                    char c = consume();
                    if (c == '\\') { // escape sequence
                        if (!peek().has_value()) {
                            std::cerr << "Unterminated escape sequence in string at line " << line << ", column " << col << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        char esc = consume();
                        switch (esc) {
                            case 'n': buf.push_back('\n'); break;
                            case 'r': buf.push_back('\r'); break;
                            case 't': buf.push_back('\t'); break;
                            case '\\': buf.push_back('\\'); break;
                            case '\'': buf.push_back('\''); break;
                            case '"': buf.push_back('"'); break;
                            default:
                                std::cerr << "Unknown escape sequence \\" << esc << " at line " << line << ", column " << col << std::endl;
                                exit(EXIT_FAILURE);
                        }
                    } else {
                        buf.push_back(c);
                    }
                }

                if (!peek().has_value()) {
                    std::cerr << "Unterminated string literal! at line " << line << ", column " << col << std::endl;
                    exit(EXIT_FAILURE);
                }

                consume(); // skip closing quote
                tokens.push_back({ .type = TokenType::STRING_LIT, .value = buf, .line = line, .col = col });
                buf.clear();
                continue;
            }
            // komentarze
            else if (peek().value() == '#' && col == 0) {
                consume();
                while (peek().has_value() && peek().value() != '\n') {
                    consume();
                }
                continue;
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
                consume();
                consume();
                while (peek().has_value() && peek().value() != '\n') {
                    consume();
                }
                continue;
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
                //std::cout << "Entered big comment section! at line " << line << ", column " << col << std::endl;
                consume();
                consume();
                while (!(peek().has_value() && peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/')) {
                    consume();
                }
                consume();
                consume();
                //std::cout << "Exited big comment section! at line " << line << ", column " << col << std::endl;
                continue;
            }
            else if (peek().value() == ';') {
                consume();
                tokens.push_back({ .type = TokenType::SEMI_COLON, .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '(') {
                consume();
                tokens.push_back({ .type = TokenType::OPEN_PAREN, .line = line, .col = col });
                continue;
            }
            else if (peek().value() == ')') {
                consume();
                tokens.push_back({ .type = TokenType::CLOSE_PAREN, .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '=') {
                consume();
                tokens.push_back({ .type = TokenType::EQUALS, .line = line, .col = col });
                continue;
            }
            else if (std::isspace(peek().value())) {
                if (peek().value() == '\n') tokens.push_back({.type = TokenType::NEW_LINE, .line = line, .col = col });
                consume();
                continue;
            } else {
                std::cerr << "Unidentified value \'" << peek().value() << "\'! at line " << line << ", column " << col << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        tokens.push_back({.type = TokenType::END_OF_FILE, .line = line, .col = col });
        m_idx = 0;
        return tokens;
    }

private:

    inline std::optional<char> peek(int offset = 0) const 
    {
        if (m_idx + offset >= m_src.length()) {
            return {};
        }
        return m_src.at(m_idx + offset);
    }

    inline char consume() {
        // first get at m_idx then increment m_idx by 1
        char c = m_src.at(m_idx++);
        if (c == '\n') {  line++; col = 0; }
        else col++;
        return c;
    }

    bool has_n_same_chars(const std::string& s, char c, int n) {
        return std::count(s.begin(), s.end(), c) == n;
    }

    size_t line = 1, col = 0;
    const std::string m_src;
    SimplifiedCommandRegistry& m_reg;
    size_t m_idx = 0;
};
