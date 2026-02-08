// backend/generator.hpp
#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <vector>

#include "./../core/scope.hpp"

namespace fs = std::filesystem;

struct Options;
struct ASTNode;

class FunctionGenerator {
public:
    FunctionGenerator(fs::path& path, Options& options, std::vector<std::shared_ptr<Scope>> variables);
    ~FunctionGenerator();

    void generate(ASTNode& node);
private:
    // implematation
    class Impl;
    std::shared_ptr<Impl> pImpl;  // shared becouse std::unique_ptr doesnt work with incomplete Impl
};
