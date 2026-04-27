// middleend/analyzer.hpp
#pragma once

#include <memory>
#include <vector>

#include "./../core/visitor.hpp"
#include "./../core/scope.hpp"

class Options;
class ASTNode;

class PreAnalyzer {
public:
    PreAnalyzer(Options& options);
    ~PreAnalyzer();

    void analyze(ASTNode& node);
    std::vector<std::shared_ptr<Scope>> getScopes() const;
private:
    // PImpl - implementation hidden in .cpp
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
