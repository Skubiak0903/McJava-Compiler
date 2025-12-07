#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include <unordered_map>

enum class TokenType {
    //say
    say,
    // scoreboards
    scoreboard,
    players,
    objectives,
    add,
    remove,
    set,
    reset,
    //selector
    selector,
    //litelals
    int_lit,
    float_lit,
    string_lit,
    // other
    semi,
    open_paren,
    close_paren, 
    equals,
    asterisk,
    // lines / file
    new_line,
    eof,
    // dynamic
    ident,
    path,
    component,
    item_predicate,
    coord,
    block_pos, // used in parser class in minecraft:block_pos parser
    rotation, // used in parser class in minecraft:block_pos parser
    json,
    item_slot,
    int_range,
    block_state,
    uuid,
};

const static std::unordered_map<std::string, TokenType> keywords = {
    //{"say", TokenType::say},
    //{"scoreboard", TokenType::scoreboard},
    //{"players", TokenType::players},
    //{"objectives", TokenType::objectives},
    //{"add", TokenType::add},
    //{"remove", TokenType::remove},
    //{"set", TokenType::set},
    //{"reset", TokenType::reset}
};

struct Token {
    TokenType type;
    std::optional<std::string> value {};
    size_t line;
    size_t col;
};

std::string tokenTypeToString(TokenType type) {
    switch(type) {
        case TokenType::add : return "add";
        case TokenType::close_paren : return ")";
        case TokenType::equals : return "=";
        case TokenType::ident : return "IDENTIFIER";
        case TokenType::int_lit : return "Int";
        case TokenType::objectives : return "objectives";
        case TokenType::open_paren : return "(";
        case TokenType::players : return "players";
        case TokenType::remove : return "remove";
        case TokenType::reset : return "reset";
        case TokenType::say : return "say";
        case TokenType::scoreboard : return "scoreboard";
        case TokenType::selector : return "SELECTOR";
        case TokenType::semi : return ";";
        case TokenType::set : return "set";
        case TokenType::string_lit : return "String"; 
        case TokenType::float_lit : return "Float"; 
        case TokenType::path : return "PATH_IDENTIFIER";
        case TokenType::asterisk : return "*";
        case TokenType::item_predicate: return "ITEM PREDICATE";
        case TokenType::block_pos: return "BLOCK POSITION";
        case TokenType::coord: return "COORD";
        case TokenType::json: return "JSON";
        case TokenType::component: return "COMPONENT";
        case TokenType::item_slot: return "ITEM SLOT";
        case TokenType::int_range: return "INT RANGE";
        case TokenType::block_state: return "BLOCK_STATE";
        case TokenType::uuid: return "UUID";
        //case TokenType::add : return "say";
        // special
        case TokenType::new_line: return "NEW_LINE";
        case TokenType::eof : return "EOF";
        default: return "UNKNOWN";
    }
}



class Tokenizer {
public:
    inline explicit Tokenizer(const std::string src)
        : m_src(std::move(src))
    {}

    std::vector<Token> tokenize() 
    {   
        std::vector<Token> tokens;
        std::string buf;
        while(peek().has_value()) {
            if (std::isalpha(peek().value())) {
                buf.push_back(consume());
                while (peek().has_value() &&
                    (std::isalnum(peek().value()) || peek().value() == '_' || peek().value() == '/' || peek().value() == ':' || peek().value() == '-' || peek().value() == '.' || peek().value() == '*' || peek().value() == '[' || peek().value() == ']'  || peek().value() == ',')) {
                    buf.push_back(consume());
                }
                if (auto it = keywords.find(buf); it != keywords.end()) {
                    tokens.push_back({.type = it->second, .line = line, .col = col });
                } else if (buf.find('/') != std::string::npos || buf.find(':') != std::string::npos) {
                    tokens.push_back({.type = TokenType::path, .value = buf, .line = line, .col = col });
                } else if (buf.find('.') != std::string::npos) {
                    tokens.push_back({.type = TokenType::item_slot, .value = buf, .line = line, .col = col });
                } else if (buf.find('[') != std::string::npos && buf.find(']') != std::string::npos) {
                    tokens.push_back({.type = TokenType::block_state, .value = buf, .line = line, .col = col });
                } else if (has_n_same_chars(buf, '-', 4)) {
                    tokens.push_back({.type = TokenType::uuid, .value = buf, .line = line, .col = col });
                } else {
                    tokens.push_back({.type = TokenType::ident, .value = buf, .line = line, .col = col });
                } 
                buf.clear();
            }
            // Lexer - pojedyncze elementy
            else if (peek().value() == '~' || peek().value() == '^') {
                std::string buf;
                buf.push_back(consume()); // ~ or ^

                // opcjonalna liczba po ~ albo ^
                if (peek().has_value() && (peek().value() == '-' || peek().value() == '.' || std::isdigit(peek().value()))) {
                    if (peek().value() == '-') buf.push_back(consume());

                    // część całkowita
                    if (peek().value() != '.')
                        while (peek().has_value() && std::isdigit(peek().value()))
                            buf.push_back(consume());

                    // opcjonalna część ułamkowa
                    if (peek().has_value() && peek().value() == '.') {
                        buf.push_back(consume()); // kropka
                        while (peek().has_value() && std::isdigit(peek().value()))
                            buf.push_back(consume());
                    }
                }

                tokens.push_back({ .type = TokenType::coord, .value = buf, .line=line, .col=col });
            }
            else if (std::isdigit(peek().value()) ||
                    (peek().value() == '-' && (std::isdigit(peek(1).value_or('0')) || peek(1).value_or('\0') == '.')) ||
                    peek().value() == '.')
            {
                std::string buf;       // pierwsza liczba
                std::string buf2;      // druga liczba, jeśli jest ".."
                bool isFloat = false;
                bool isRange = false;

                // minus opcjonalny
                if (peek().value() == '-') buf.push_back(consume());

                // część całkowita
                while (peek().has_value() && std::isdigit(peek().value()))
                    buf.push_back(consume());

                // opcjonalna część ułamkowa
                if (peek().has_value() && peek().value() == '.') {
                    // sprawdzamy czy to ".." (range) czy float
                    if (peek(1).has_value() && peek(1).value() == '.') {
                        // range
                        isRange = true;
                        consume(); // pierwsza kropka
                        consume(); // druga kropka

                        // druga liczba w zakresie (opcjonalna)
                        if (peek().has_value() && (peek().value() == '-' || std::isdigit(peek().value()))) {
                            if (peek().value() == '-') buf2.push_back(consume());
                            while (peek().has_value() && std::isdigit(peek().value()))
                                buf2.push_back(consume());
                        }
                    } else {
                        // float
                        isFloat = true;
                        buf.push_back(consume()); // kropka
                        while (peek().has_value() && std::isdigit(peek().value()))
                            buf.push_back(consume());
                    }
                }

                // tworzymy token
                if (isRange) {
                    tokens.push_back({ .type = TokenType::int_range,
                                    .value = buf + ".." + buf2,
                                    .line = line, .col = col });
                } else if (isFloat) {
                    tokens.push_back({ .type = TokenType::float_lit,
                                    .value = buf,
                                    .line = line, .col = col });
                } else {
                    tokens.push_back({ .type = TokenType::int_lit,
                                    .value = buf,
                                    .line = line, .col = col });
                }
            }
            else if (peek().value() == '@') {
                buf.clear();
                buf.push_back(consume());
                if (
                    peek().has_value() && (
                        peek().value() == 's' || 
                        peek().value() == 'a' || 
                        peek().value() == 'p' || 
                        peek().value() == 'r' || 
                        peek().value() == 'e' || 
                        peek().value() == 'n'
                    )) 
                {   
                    buf.push_back(consume());
                    if (peek().has_value() && peek().value() == '[') {
                        buf.push_back(consume());
                        while (peek().has_value() && (peek().value() != ']'  && peek().value() != '\n')) {
                            buf.push_back(consume());
                        }
                        //std::cout << "DEBUG: Selector parsed: " << buf << std::endl;
                        if (!peek().has_value() || !peek().value() == ']') {
                            std::cerr << "Expected ']'! at line " << line << ", column " << col << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        buf.push_back(consume());
                        //std::cout << "DEBUG: Selector parsed: " << buf << std::endl;
                        if (peek().has_value() && std::isspace(peek().value())) {
                            tokens.push_back({ .type = TokenType::selector, .value = buf, .line = line, .col = col });
                            if (peek().value() == '\n') tokens.push_back({.type = TokenType::new_line, .line = line, .col = col });
                            consume();
                        } else {
                            std::cerr << "Invalid Selector! at line " << line << ", column " << col << std::endl;
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        if (peek().has_value() && std::isspace(peek().value())) {
                            tokens.push_back({ .type = TokenType::selector, .value = buf, .line = line, .col = col });
                            if (peek().value() == '\n') tokens.push_back({.type = TokenType::new_line, .line = line, .col = col });
                            consume();
                        } else {
                            std::cerr << "Invalid Selector! at line " << line << ", column " << col << std::endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                    
                    buf.clear();
                    continue;

                } else {
                    std::cerr << "Invalid Selector! at line " << line << ", column " << col << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else if (peek().value() == '{' || peek().value() == '[') {
                char open = consume();           // '{' lub '['
                char close = (open == '{') ? '}' : ']';
                int depth = 1;
                buf.clear();
                buf.push_back(open);

                while (peek().has_value() && depth > 0) {
                    char c = consume();
                    buf.push_back(c);

                    // jeśli trafiamy na string, pomijamy zawartość w środku
                    if (c == '"' || c == '\'') {
                        char quote = c;
                        while (peek().has_value() && peek().value() != quote) {
                            char s = consume();
                            buf.push_back(s);
                            if (s == '\\' && peek().has_value()) { // escape
                                buf.push_back(consume());
                            }
                        }
                        if (peek().has_value()) buf.push_back(consume()); // zamykający quote
                    }
                    else if (c == open) {
                        depth++;
                    }
                    else if (c == close) {
                        depth--;
                    }
                }

                if (depth != 0) {
                    std::cerr << "Unterminated JSON structure! Expected '" << close
                            << "' at line " << line << ", column " << col << std::endl;
                    exit(EXIT_FAILURE);
                }

                tokens.push_back({
                    .type = (open == '{') ? TokenType::component : TokenType::json,
                    .value = buf,
                    .line = line,
                    .col = col
                });
                buf.clear();
                continue;
            }
            else if (peek().value() == '%') {
                buf.clear();
                buf.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value())) {
                    buf.push_back(consume());
                }
                tokens.push_back({ .type = TokenType::selector, .value = buf, .line = line, .col = col });
                buf.clear();
                continue;
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
                tokens.push_back({ .type = TokenType::string_lit, .value = buf, .line = line, .col = col });
                buf.clear();
                continue;
            }
            else if (peek().value() == '#' && col != 0) {
                buf.clear();
                buf.push_back(consume());
                while (peek().has_value() && (std::isalnum(peek().value()) || peek().value() == '_' || peek().value() == ':' || peek().value() == '/')) {
                    buf.push_back(consume());
                }
                tokens.push_back({ .type = TokenType::item_predicate, .value = buf, .line = line, .col = col });
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
            if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
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
                tokens.push_back({ .type = TokenType::semi, .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '>' && peek(1).has_value() && peek(1).value() == '=') {
                consume();
                consume();
                tokens.push_back({ .type = TokenType::ident, .value = ">=", .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '<' && peek(1).has_value() && peek(1).value() == '=') {
                consume();
                consume();
                tokens.push_back({ .type = TokenType::ident, .value = "<=", .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '<') {
                consume();
                tokens.push_back({ .type = TokenType::ident, .value = "<", .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '>') {
                consume();
                tokens.push_back({ .type = TokenType::ident, .value = ">", .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '=') {
                consume();
                tokens.push_back({ .type = TokenType::ident, .value = "=", .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '(') {
                consume();
                tokens.push_back({ .type = TokenType::open_paren, .line = line, .col = col });
                continue;
            }
            else if (peek().value() == ')') {
                consume();
                tokens.push_back({ .type = TokenType::close_paren, .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '=') {
                consume();
                tokens.push_back({ .type = TokenType::equals, .line = line, .col = col });
                continue;
            }
            else if (peek().value() == '*') {
                consume();
                tokens.push_back({ .type = TokenType::asterisk, .line = line, .col = col });
                continue;
            }
            else if (std::isspace(peek().value())) {
                if (peek().value() == '\n') tokens.push_back({.type = TokenType::new_line, .line = line, .col = col });
                consume();
                continue;
            } else {
                std::cerr << "Unidentified value \'" << peek().value() << "\'! at line " << line << ", column " << col << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        tokens.push_back({.type = TokenType::eof, .line = line, .col = col });
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
        if (c == '\r') {}
        else if (c == '\n') {  line++; col = 0; }
        else col++;
        return c;
    }

    bool has_n_same_chars(const std::string& s, char c, int n) {
        return std::count(s.begin(), s.end(), c) == n;
    }

    size_t line = 1, col = 0;
    const std::string m_src;
    size_t m_idx = 0;
};