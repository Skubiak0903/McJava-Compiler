// core/ast.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>

#include "./token.hpp"
#include "./varInfo.hpp"
#include "./visitor.hpp"


struct Annotation {
    std::string name;
    // in the future -> annotation arguments -> @Annotation(type = "special") or something similar
};


// ========== CLASSES ==========
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual ASTReturn accept(ASTVisitor& visitor) const = 0;

    template<typename T>
    T visit(ASTVisitor& visitor) const {
        return std::get<T>(this->accept(visitor));
    }

    
    // --- Semantic Data ---
    mutable bool isAnalyzed = false;
    std::vector<Annotation> annotations = {};
};



// ===== SUBCLASSES =====

class CommandNode : public ASTNode {
public:
    Token command;
    std::vector<std::unique_ptr<ASTNode>> args;
    
    CommandNode(Token cmd, std::vector<std::unique_ptr<ASTNode>> args = {})
        : command(cmd), args(std::move(args)) {}

    ASTReturn accept(ASTVisitor& visitor) const override {
        return visitor.visitCommand(*this);
    }
};


class VarDeclNode : public ASTNode {
public:
    Token name;
    std::unique_ptr<ASTNode> value;
 
    mutable std::shared_ptr<VarInfo> varInfo;
    
    VarDeclNode(Token name, std::unique_ptr<ASTNode> value)
        : name(name), value(std::move(value)) {}

    ASTReturn accept(ASTVisitor& visitor) const override {
        return visitor.visitVarDecl(*this);
    }
};


class ExprNode : public ASTNode {
public:
    Token token;
    
    mutable bool forceDynamic = false;
    mutable std::shared_ptr<VarInfo> varInfo;
    
    ExprNode(Token token)
        : token(token) {}
    
    ASTReturn accept(ASTVisitor& visitor) const override {
        return visitor.visitExpr(*this);
    }
};


class BinaryOpNode : public ASTNode {
public:
    Token op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    mutable std::shared_ptr<VarInfo> varInfo;
    
    BinaryOpNode(Token op, std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    ASTReturn accept(ASTVisitor& visitor) const override {
        return visitor.visitBinaryOp(*this);
    }
};


class IfNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;  // can be nullptr

    mutable bool isConditionConstant = false;
    mutable bool conditionValue = false;
    
    IfNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> thenBranch, std::unique_ptr<ASTNode> elseBranch)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    
    ASTReturn accept(ASTVisitor& visitor) const override {
        return visitor.visitIf(*this);
    }
};


class WhileNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;

    mutable bool isConditionConstant = false;
    mutable bool conditionValue = false;
    
    WhileNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body)
        : condition(std::move(condition)), body(std::move(body)) {}
    
    ASTReturn accept(ASTVisitor& visitor) const override {
        return visitor.visitWhile(*this);
    }
};


class ScopeNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    
    ScopeNode(std::vector<std::unique_ptr<ASTNode>> statements = {})
        : statements(std::move(statements)) {}
    
    ASTReturn accept(ASTVisitor& visitor) const override {
        return visitor.visitScope(*this);
    }
};