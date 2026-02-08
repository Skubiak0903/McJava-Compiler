// frontend/parser.hpp
#pragma once

#include <vector>
#include <memory>

class SimplifiedCommandRegistry;
struct Token;
struct ASTNode;

class Parser {
public:
    Parser(const std::vector<Token> tokens, SimplifiedCommandRegistry& reg);
    ~Parser();
    
    std::unique_ptr<ASTNode> parse();

private:
    // implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
