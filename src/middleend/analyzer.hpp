// middleend/analyzer.hpp
#pragma once

#include <memory>
#include <unordered_map>

#include "./../core/visitor.hpp"


class Options;
class ASTNode;

class Analyzer {
public:
    Analyzer(Options& options);
    ~Analyzer();

    void analyze(ASTNode& node);
    std::unordered_map<std::string, std::shared_ptr<VarInfo>> getVariables() const;
private:
    // PImpl - implementation hidden in .cpp
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
