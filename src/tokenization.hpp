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
    // Literały
    INT_LIT, FLOAT_LIT, STRING_LIT,
    
    // Operatory
    PLUS, MINUS, MULTIPLY, DIVIDE,
    EQUALS, EQUALS_EQUALS, NOT_EQUALS,
    LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    
    // Nawiasy
    OPEN_PAREN, CLOSE_PAREN,
    OPEN_BRACE, CLOSE_BRACE,
    OPEN_BRACKET, CLOSE_BRACKET,
    
    // Interpunkcja
    SEMI_COLON, COMMA, DOT,
    
    // Słowa kluczowe
    WHILE, FOR, IF, ELSE, RETURN, TRUE, FALSE,
    
    // Dynamiczne
    IDENT, CMD_KEY,
    
    // Specjalne
    NEW_LINE, END_OF_FILE,
};

const inline std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"return", TokenType::RETURN},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
};

const inline std::unordered_map<char, TokenType> CHARS = {
    {'+', TokenType::PLUS},
    {'-', TokenType::MINUS},
    {'*', TokenType::MULTIPLY},
    {'/', TokenType::DIVIDE},

    {'=', TokenType::EQUALS},
    {'<', TokenType::LESS},
    {'>', TokenType::GREATER},

    {'(', TokenType::OPEN_PAREN},
    {')', TokenType::CLOSE_PAREN},
    {'{', TokenType::OPEN_BRACE},
    {'}', TokenType::CLOSE_BRACE},
    {'[', TokenType::OPEN_BRACKET},
    {']', TokenType::CLOSE_BRACKET},

    {';', TokenType::SEMI_COLON},
    {',', TokenType::COMMA},
    {'.', TokenType::DOT},
};

const inline std::unordered_map<std::string, TokenType> DOUBLE_CHARS = {
    {"==", TokenType::EQUALS_EQUALS},
    {"!=", TokenType::NOT_EQUALS},
    {"<=", TokenType::LESS_EQUAL},
    {">=", TokenType::GREATER_EQUAL},
};



inline std::string tokenTypeToString(TokenType type) {
    switch(type) {
        //litelals
        case TokenType::INT_LIT : return "Int";
        case TokenType::FLOAT_LIT : return "Float"; 
        case TokenType::STRING_LIT : return "String";
        
        // Operatory
        case TokenType::PLUS : return "PLUS (+)";
        case TokenType::MINUS : return "MINUS (-)";
        case TokenType::MULTIPLY : return "MULTIPLY (*)";
        case TokenType::DIVIDE : return "DIVIDE (/)";
        case TokenType::EQUALS : return "EQUALS (=)";
        case TokenType::EQUALS_EQUALS : return "EQUAL (==)";
        case TokenType::NOT_EQUALS : return "NOT EQUAL (!=)";
        case TokenType::LESS : return "LESS (<)";
        case TokenType::GREATER : return "GREATER (>)";
        case TokenType::LESS_EQUAL : return "LESS OR EQUAL (<=)";
        case TokenType::GREATER_EQUAL : return "GREATER OR EQUAL (>=)";

        // Nawiasy
        case TokenType::OPEN_PAREN : return "(";
        case TokenType::CLOSE_PAREN : return ")";
        case TokenType::OPEN_BRACE : return "{";
        case TokenType::CLOSE_BRACE : return "}";
        case TokenType::OPEN_BRACKET : return "[";
        case TokenType::CLOSE_BRACKET : return "]";
        
        // Interpunkcja
        case TokenType::SEMI_COLON : return ";";
        case TokenType::COMMA : return ",";
        case TokenType::DOT : return ".";

        
        // Słowa kluczowe
        case TokenType::WHILE : return "while";
        case TokenType::FOR : return "for";
        case TokenType::IF : return "if";
        case TokenType::ELSE : return "else";
        case TokenType::RETURN : return "return";
        case TokenType::TRUE : return "true";
        case TokenType::FALSE : return "false";

        // dynamic tokens
        case TokenType::IDENT : return "IDENTIFIER";
        case TokenType::CMD_KEY : return "COMMAND_KEY";
        
        // special
        case TokenType::NEW_LINE: return "NEW_LINE";
        case TokenType::END_OF_FILE : return "END_OF_FILE";
        default: return "UNKNOWN";
    }
}

struct Token {
    TokenType type;
    std::optional<std::string> value {};
    size_t line;
    size_t col;
};

class Tokenizer {
public:
    inline explicit Tokenizer(const std::string src, SimplifiedCommandRegistry& registry)
        : m_src(std::move(src)), m_reg(registry) {}

    std::vector<Token> tokenize() 
    {   
        std::vector<Token> tokens;
        std::string buf;
        while(peek().has_value()) {

            char value = peek().value();

            // keywords & idents
            if (std::isalpha(value)) {
                buf.push_back(consume());
                while (peek().has_value() &&
                    (std::isalnum(peek().value()) || peek().value() == '_' || peek().value() == '-')) {
                    buf.push_back(consume());
                }

                // check if word is keyword
                auto it = KEYWORDS.find(buf);
                if (it != KEYWORDS.end()) {
                    tokens.push_back({ .type = it->second, .line = line, .col = col });
                    
                    buf.clear();
                    continue;
                }

                if (m_reg.isValid(buf)) {
                    tokens.push_back({.type = TokenType::CMD_KEY, .value = buf, .line = line, .col = col });
                } else {
                    tokens.push_back({.type = TokenType::IDENT, .value = buf, .line = line, .col = col });
                } 

                buf.clear();
                continue;
            }

            // numbers
            if (std::isdigit(value) ||
                    (value == '-' && (std::isdigit(peek(1).value_or('0')) || peek(1).value_or('\0') == '.')) ||
                    value == '.')
            {
                std::string buf;       // pierwsza liczba
                bool isFloat = false;

                // minus opcjonalny
                if (value == '-') buf.push_back(consume());

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

                continue;
            }

            // strings 
            if (value == '"' || value == '\'') {
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

            // comments
            if (value == '#' && col == 0) {
                consume();
                while (peek().has_value() && peek().value() != '\n') {
                    consume();
                }
                continue;
            }

            if (peek(1).has_value()) {
                char value2 = peek(1).value();

                if (value == '/' && value2 == '/') {
                    consume();
                    consume();
                    while (peek().has_value() && peek().value() != '\n') {
                        consume();
                    }
                    continue;
                }
    
                if (value == '/' && value2 == '*') {
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
    
                // check for double char tokens
                std::string doubleChar;
                doubleChar += value;
                doubleChar += value2;

                auto it = DOUBLE_CHARS.find(doubleChar);
                if (it != DOUBLE_CHARS.end()) {
                    consume();
                    consume();
                    
                    tokens.push_back({ .type = it->second, .value = doubleChar, .line = line, .col = col });
                    
                    buf.clear();
                    continue;
                }
            }

            // check for single char tokens
            auto it = CHARS.find(value);
            if (it != CHARS.end()) {
                consume();
                
                std::string str(1, value);
                tokens.push_back({ .type = it->second, .value = str, .line = line, .col = col });
                continue;
            }

            // new line
            if (std::isspace(value)) {
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
