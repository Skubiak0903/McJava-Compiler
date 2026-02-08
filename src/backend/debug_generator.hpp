// backend/debug_generator.hpp
#pragma once

#include <memory>
#include <ostream>

#include "./../core/visitor.hpp"

struct ASTNode;

class DebugGenerator {
public:
    DebugGenerator(std::ostream& out);
    ~DebugGenerator();

    void generate(ASTNode& node);
private:
    // implementation
    class Impl;
    std::shared_ptr<Impl> pImpl; // shared because std::unique_ptr doesnt work with incomplete Impl or smth like this
};
