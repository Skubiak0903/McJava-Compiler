// frontend/tokenizer.cpp
#include "./tokenizer.hpp"

#include <iostream>
#include <unordered_map>
#include <cctype>

#include "./../registries/SimplifiedCommandRegistry.hpp"
#include "./../core/token.hpp"



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


// Tokenizer implementation from ./tokenizer.hpp
class Tokenizer::Impl {
private:
    size_t line = 1, col = 0;
    std::string m_src;
    SimplifiedCommandRegistry& m_reg;
    size_t m_idx = 0;


    inline std::optional<char> peek(int offset = 0) const 
    {
        if (m_idx + offset >= m_src.length()) {
            return {};
        }
        // we are sure that this value exists
        return m_src[m_idx + offset];
    }

    inline char consume() {
        // m_idx++ -> first get at m_idx then increment m_idx by 1
        char c = m_src.at(m_idx++);
        if (c == '\n') {  line++; col = 0; }
        else col++;
        return c;
    }
    
public:
    Impl(const std::string& src, SimplifiedCommandRegistry& registry)
        : m_src(src), m_reg(registry) {}
    
    std::vector<Token> tokenize() 
    {   
        std::vector<Token> tokens;
        std::string buf;
        while(peek().has_value()) {

            char value = peek().value();

            // keywords & idents
            if (std::isalpha(value)) {
                size_t start_line = line, start_col = col;

                buf.push_back(consume());
                while (peek().has_value() &&
                    (std::isalnum(peek().value()) || peek().value() == '_' || peek().value() == '-')) {
                    buf.push_back(consume());
                }

                // check if word is keyword
                auto it = KEYWORDS.find(buf);
                if (it != KEYWORDS.end()) {
                    tokens.push_back({ .type = it->second, .line = start_line, .col = start_col });
                    
                    buf.clear();
                    continue;
                }

                if (m_reg.isValid(buf)) {
                    tokens.push_back({.type = TokenType::CMD_KEY, .value = buf, .line = start_line, .col = start_col });
                } else {
                    tokens.push_back({.type = TokenType::IDENT, .value = buf, .line = start_line, .col = start_col });
                } 

                buf.clear();
                continue;
            }
            
            // numbers
            if (std::isdigit(value) ||
                    (value == '-' && peek(1).has_value() && std::isdigit(peek(1).value())) ||
                    (value == '.' && peek(1).has_value() && std::isdigit(peek(1).value())) ||
                    (value == '-' && peek(1).has_value() && peek(1).value() == '.' && peek(2).has_value() && std::isdigit(peek(2).value())))
            {
                // reuse outer buf, already cleared
                bool isFloat = false;

                // optional minus
                if (value == '-') buf.push_back(consume());

                // integer part
                while (peek().has_value() && std::isdigit(peek().value()))
                    buf.push_back(consume());

                // check if the value is float
                if (peek().has_value() && peek().value() == '.') {
                    isFloat = true;
                    buf.push_back(consume()); // consume '.'
                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buf.push_back(consume());
                    }
                }

                if (isFloat) {
                    tokens.push_back({ .type = TokenType::FLOAT_LIT,
                                    .value = buf,
                                    .line = line, .col = col });
                } else {
                    tokens.push_back({ .type = TokenType::INT_LIT,
                                    .value = buf,
                                    .line = line, .col = col });
                }

                buf.clear();
                continue;
            }

            // strings 
            if (value == '"' || value == '\'') {
                char quote = consume(); // consume " or '
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

            // annotations
            if (value == '@') {
                consume(); // consume '@'
                while (peek().has_value() && (std::isalnum(peek().value()) || peek().value() == '_')) {
                    buf.push_back(consume());
                }

                if (buf.empty()) {
                    std::cerr << "Empty annotation name at line " << line << ", column " << col << std::endl;
                    exit(EXIT_FAILURE);
                }

                tokens.push_back({ .type = TokenType::ANNOTATION, .value = buf, .line = line, .col = col });
                buf.clear();
                continue;
            }

            // comments
            if (value == '#' && col == 0) {
                consume(); //  consume '#'
                while (peek().has_value() && peek().value() != '\n') {
                    consume();
                }
                continue;
            }

            if (peek(1).has_value()) {
                char value2 = peek(1).value();

                if (value == '/' && value2 == '/') {
                    consume(); // consume '/'
                    consume(); // consume '/'
                    while (peek().has_value() && peek().value() != '\n') {
                        consume();
                    }
                    continue;
                }
    
                if (value == '/' && value2 == '*') {
                    consume(); // consume '/'
                    consume(); // consume '*'

                    while (peek().has_value() && !(peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/')) {
                        consume();
                    }
                    
                    if (!peek().has_value()) {
                        std::cerr << "Unterminated block comment at line " << line << ", column " << col << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    
                    // we made sure that the next 2 chars are '*' and '/' -> while condition is defining it
                    consume(); // consume '*'
                    consume(); // consume '/'
                    continue;
                }
    
                // check for double char tokens
                std::string doubleChar;
                doubleChar += value;
                doubleChar += value2;

                auto it = DOUBLE_CHARS.find(doubleChar);
                if (it != DOUBLE_CHARS.end()) {
                    consume(); // consume the first character of DOUBLE_CHAR
                    consume(); // consume the secon character of DOUBLE_CHAR
                    
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
        line = 1;
        col = 0;
        return tokens;
    }
};

// ========== WRAPPER ==========
Tokenizer::Tokenizer(const std::string& source, SimplifiedCommandRegistry& registry)
    : pImpl(std::make_unique<Impl>(source, registry)) {}

Tokenizer::~Tokenizer() = default;  // Needed for unique_ptr<Impl>

std::vector<Token> Tokenizer::tokenize() {
    return pImpl->tokenize();
}