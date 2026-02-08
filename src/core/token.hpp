// core/token.hpp
#pragma once

#include <string>
#include <optional>

enum class TokenType {
    // Literals
    INT_LIT, FLOAT_LIT, STRING_LIT,
    
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE,
    EQUALS, EQUALS_EQUALS, NOT_EQUALS,
    LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    
    // Brackets
    OPEN_PAREN, CLOSE_PAREN,
    OPEN_BRACE, CLOSE_BRACE,
    OPEN_BRACKET, CLOSE_BRACKET,
    
    // Punctuation
    SEMI_COLON, COMMA, DOT,
    
    // Keywords
    WHILE, FOR, IF, ELSE, RETURN, TRUE, FALSE,
    
    // Dynamic
    IDENT, CMD_KEY, ANNOTATION,
    
    // Special
    NEW_LINE, END_OF_FILE,
};

const inline std::string tokenTypeToString(const TokenType type) {
    switch(type) {
        // Literals
        case TokenType::INT_LIT     : return "Int";
        case TokenType::FLOAT_LIT   : return "Float"; 
        case TokenType::STRING_LIT  : return "String";
        
        // Operators
        case TokenType::PLUS            : return "PLUS (+)";
        case TokenType::MINUS           : return "MINUS (-)";
        case TokenType::MULTIPLY        : return "MULTIPLY (*)";
        case TokenType::DIVIDE          : return "DIVIDE (/)";
        case TokenType::EQUALS          : return "EQUALS (=)";
        case TokenType::EQUALS_EQUALS   : return "EQUAL TO (==)";
        case TokenType::NOT_EQUALS      : return "NOT EQUAL TO (!=)";
        case TokenType::LESS            : return "LESS (<)";
        case TokenType::GREATER         : return "GREATER (>)";
        case TokenType::LESS_EQUAL      : return "LESS OR EQUAL (<=)";
        case TokenType::GREATER_EQUAL   : return "GREATER OR EQUAL (>=)";

        // Brackets
        case TokenType::OPEN_PAREN      : return "(";
        case TokenType::CLOSE_PAREN     : return ")";
        case TokenType::OPEN_BRACE      : return "{";
        case TokenType::CLOSE_BRACE     : return "}";
        case TokenType::OPEN_BRACKET    : return "[";
        case TokenType::CLOSE_BRACKET   : return "]";
        
        // Interpunction
        case TokenType::SEMI_COLON  : return ";";
        case TokenType::COMMA       : return ",";
        case TokenType::DOT         : return ".";

        
        // Keywords
        case TokenType::WHILE   : return "while";
        case TokenType::FOR     : return "for";
        case TokenType::IF      : return "if";
        case TokenType::ELSE    : return "else";
        case TokenType::RETURN  : return "return";
        case TokenType::TRUE    : return "true";
        case TokenType::FALSE   : return "false";

        // Dynamic
        case TokenType::IDENT       : return "IDENTIFIER";
        case TokenType::CMD_KEY     : return "COMMAND_KEY";
        case TokenType::ANNOTATION  : return "ANNOTATION";
        
        // Special
        case TokenType::NEW_LINE    : return "NEW_LINE";
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