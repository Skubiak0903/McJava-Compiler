// frontend/tokenizer.hpp
#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>

class SimplifiedCommandRegistry;
struct Token; 

class Tokenizer {
public:
    Tokenizer(const std::string& src, SimplifiedCommandRegistry& registry);
    ~Tokenizer();
    
    std::vector<Token> tokenize();

private:
    // implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
