// visitor.hpp
#pragma once

#include <iostream>
#include <any>

// Forward declarations
class CommandNode;
class VarDeclNode;
class ExprNode;
class BinaryOpNode;
class IfNode;
class WhileNode;
class ScopeNode;

// Non-templated base visitor using type-erased results
class ASTVisitorBase {
public:
    virtual ~ASTVisitorBase() = default;

    virtual std::any visitCommand(const CommandNode& node) = 0;
    virtual std::any visitVarDecl(const VarDeclNode& node) = 0;
    virtual std::any visitExpr(const ExprNode& node) = 0;
    virtual std::any visitBinaryOp(const BinaryOpNode& node) = 0;
    virtual std::any visitIf(const IfNode& node) = 0;
    virtual std::any visitWhile(const WhileNode& node) = 0;
    virtual std::any visitScope(const ScopeNode& node) = 0;
};

// Templated visitor that concrete generators implement. They implement
// typed visit*T(...) methods; the base overrides wrap/unwrap via std::any.
template<typename T = void>
class ASTVisitor : public ASTVisitorBase {
public:
    virtual ~ASTVisitor() = default;

    virtual T visitCommandT(const CommandNode& node) = 0;
    virtual T visitVarDeclT(const VarDeclNode& node) = 0;
    virtual T visitExprT(const ExprNode& node) = 0;
    virtual T visitBinaryOpT(const BinaryOpNode& node) = 0;
    virtual T visitIfT(const IfNode& node) = 0;
    virtual T visitWhileT(const WhileNode& node) = 0;
    virtual T visitScopeT(const ScopeNode& node) = 0;

    // Implement non-templated base methods to allow dynamic dispatch
    std::any visitCommand(const CommandNode& node) override final { return visitCommandT(node); }
    std::any visitVarDecl(const VarDeclNode& node) override final { return visitVarDeclT(node); }
    std::any visitExpr(const ExprNode& node) override final { return visitExprT(node); }
    std::any visitBinaryOp(const BinaryOpNode& node) override final { return visitBinaryOpT(node); }
    std::any visitIf(const IfNode& node) override final { return visitIfT(node); }
    std::any visitWhile(const WhileNode& node) override final { return visitWhileT(node); }
    std::any visitScope(const ScopeNode& node) override final { return visitScopeT(node); }
};

// Specialization for void result type: typed visit methods return void,
// base overrides call them and return an empty std::any.
template<>
class ASTVisitor<void> : public ASTVisitorBase {
public:
    virtual ~ASTVisitor() = default;

    virtual void visitCommandT(const CommandNode& node) = 0;
    virtual void visitVarDeclT(const VarDeclNode& node) = 0;
    virtual void visitExprT(const ExprNode& node) = 0;
    virtual void visitBinaryOpT(const BinaryOpNode& node) = 0;
    virtual void visitIfT(const IfNode& node) = 0;
    virtual void visitWhileT(const WhileNode& node) = 0;
    virtual void visitScopeT(const ScopeNode& node) = 0;

    std::any visitCommand(const CommandNode& node) override final { visitCommandT(node); return std::any{}; }
    std::any visitVarDecl(const VarDeclNode& node) override final { visitVarDeclT(node); return std::any{}; }
    std::any visitExpr(const ExprNode& node) override final { visitExprT(node); return std::any{}; }
    std::any visitBinaryOp(const BinaryOpNode& node) override final { visitBinaryOpT(node); return std::any{}; }
    std::any visitIf(const IfNode& node) override final { visitIfT(node); return std::any{}; }
    std::any visitWhile(const WhileNode& node) override final { visitWhileT(node); return std::any{}; }
    std::any visitScope(const ScopeNode& node) override final { visitScopeT(node); return std::any{}; }
};