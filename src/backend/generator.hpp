// backend/generator.hpp
#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <unordered_map>

#include "./../core/visitor.hpp"

namespace fs = std::filesystem;
struct Options;
struct ASTNode;

class FunctionGenerator {
public:
    FunctionGenerator(fs::path& path, Options& options, std::unordered_map<std::string, std::shared_ptr<VarInfo>> variables);
    ~FunctionGenerator();

    void generate(ASTNode& node);
private:
    // implematation
    class Impl;
    std::shared_ptr<Impl> pImpl;  // shared becouse std::unique_ptr doesnt work with incomplete Impl
};
