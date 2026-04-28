// middleend/analyzer.cpp
#include "./preAnalyzer.hpp"

#include <iostream>

#include "./../core/ast.hpp"
#include "./../core/options.hpp"

class PreAnalyzer::Impl : public ASTVisitor {
public:
    Impl(Options& options) : 
        options_(options) {}

    void analyze(ASTNode& node) {
        node.accept(*this);
    }

    ASTReturn visitCommand(const CommandNode& node) override {
        analyzeCommand(node);
        return nullptr;
    }

    ASTReturn visitVarDecl(const VarDeclNode& node) override {
        analyzeVarDecl(node);
        return nullptr;
    }

    ASTReturn visitVarAssign(const VarAssignNode& node) override {
        analyzeVarAssign(node);
        return nullptr;
    }

    ASTReturn visitExpr(const ExprNode& node) override {
        analyzeExpr(node); 
        return nullptr;
    }

    ASTReturn visitBinaryOp(const BinaryOpNode& node) override {
        analyzeBinaryOp(node);
        return nullptr;
    }

    ASTReturn visitIf(const IfNode& node) override {
        analyzeIf(node);
        return nullptr;
    }

    ASTReturn visitWhile(const WhileNode& node) override {
        analyzeWhile(node);
        return nullptr;
    }

    ASTReturn visitScope(const ScopeNode& node) override {
        analyzeScope(node);
        return nullptr;
    }

    ASTReturn visitFuncDecl(const FuncDeclNode& node) override {
        analyzeFuncDecl(node);
        return nullptr;
    }

    ASTReturn visitFuncCall(const FuncCallNode& node) override {
        analyzeFuncCall(node);
        return nullptr;
    }

private:
    //std::vector<std::shared_ptr<Scope>> allScopes_;
    std::vector<std::shared_ptr<Scope>> scopeStack_;
    // size_t nextScopeId_ = 0; in scope.hpp

    size_t nextFunctionId_ = 0;
    
    Options& options_;

    /*Scope& getCurrentScope() {
        if (scopeStack_.empty()) error("Tried to access empty scope stack");
        return *scopeStack_.back();
    }*/

    std::shared_ptr<Scope> getCurrentScope() {
        if (scopeStack_.empty()) error("Tried to access empty scope stack");
        return scopeStack_.back();
    }


    void enterScope() {
        auto newScope = std::make_shared<Scope>();

        newScope->id = nextScopeId++;
        newScope->name = "scope_" + std::to_string(newScope->id);
        newScope->parent = scopeStack_.empty() ? nullptr : scopeStack_.back();
        newScope->isRoot = false;

        //allScopes_.push_back(newScope);
        scopeStack_.push_back(newScope);
    }

    void exitScope() {
        if (scopeStack_.empty()) error("Tried to exit scope but scope stack is empty");
        scopeStack_.pop_back(); 
    }

    inline std::shared_ptr<VarInfo> visit(ASTNode& node) { return node.visit<std::shared_ptr<VarInfo>>(*this); }

    void analyzeCommand(const CommandNode& node) {
        node.scope = getCurrentScope();
        for (const auto& arg : node.args) {
            visit(*arg); // Analyze all expressions
        }
    }

    void analyzeVarDecl(const VarDeclNode& node) {
        node.scope = getCurrentScope();
        visit(*node.value);
    }

    void analyzeVarAssign(const VarAssignNode& node) {
        node.scope = getCurrentScope();
        visit(*node.value);
    }

    void analyzeExpr(const ExprNode& node) {
        node.scope = getCurrentScope();
    }

    void analyzeBinaryOp(const BinaryOpNode& node){
        node.scope = getCurrentScope();
        visit(*node.left);
        visit(*node.right);
    }

    void analyzeIf(const IfNode& node) {
        node.scope = getCurrentScope();
        visit(*node.condition);
        visit(*node.thenBranch);
        if (node.elseBranch) visit(*node.elseBranch);
    }

    void analyzeWhile(const WhileNode& node) {
        node.scope = getCurrentScope();        
        visit(*node.condition);
        visit(*node.body);
    }

    void analyzeScope(const ScopeNode& node) {
        if (scopeStack_.empty()) {
            // root scope
            auto rootScope = std::make_shared<Scope>();

            rootScope->id = nextScopeId++;
            rootScope->name = "rootScope";
            rootScope->parent = nullptr;
            rootScope->isRoot = true;

            scopeStack_.push_back(rootScope);

            node.scope = rootScope;
            
            for (const auto& arg : node.statements) {
                visit(*arg); // Analyze all nodes
            }

        } else {
            node.scope = getCurrentScope();

            enterScope();
            for (const auto& arg : node.statements) {
                visit(*arg); // Analyze all nodes
            }
            exitScope();
        }

    }

    void analyzeFuncDecl(const FuncDeclNode& node) {
        node.scope = getCurrentScope();
        
        // Predeclare functions
        auto funcName = node.name.value.value();
        
        FuncInfo funcData = {
            .name = funcName,
            .scopeName = "function_" + std::to_string(nextFunctionId_++),
            .isUsed = false,
        };
        
        auto funcInfo = std::make_shared<FuncInfo>(funcData);
        
        bool declared = getCurrentScope()->declareFunc(funcName, funcInfo);
        if (!declared) {
            error("Function with name '" + funcName + "' already exists!");
        }

        visit(*node.body);
    }

    void analyzeFuncCall(const FuncCallNode& node) {
        node.scope = getCurrentScope();
    }

private:
    [[noreturn]] void error(const std::string& msg) {
        std::cerr << "PreAnalyzer error: " << msg << std::endl;
        exit(EXIT_FAILURE);
    }
};


// ========== WRAPPER ==========
PreAnalyzer::PreAnalyzer(Options& options)
    : pImpl(std::make_unique<Impl>(options)) {}

PreAnalyzer::~PreAnalyzer() = default; // Needed for unique_ptr<Impl>

void PreAnalyzer::analyze(ASTNode& node) {
    pImpl->analyze(node);
}