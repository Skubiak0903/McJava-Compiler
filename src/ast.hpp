// ast.hpp
#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <iostream>

#include "tokenization.hpp"
#include "visitor.hpp"
#include <any>

// ----------------------------------------------------
// PROSTE AST Z DZIEDZICZENIEM (jak w Javie!)
// ----------------------------------------------------


enum class DataType {
    INT, FLOAT, BOOL, STRING, UNKNOWN
};

// Podstawowa klasa AST (jak interface w Javie)
class ASTNode {
public:
    virtual ~ASTNode() = default;

    template<typename T>
    T generate(ASTVisitor<T>& visitor) const {
        return std::any_cast<T>(accept(visitor));
    }

    // Overload for void-result visitors so we don't attempt std::any_cast<void>.
    void generate(ASTVisitor<void>& visitor) const {
        accept(visitor);
    }

    DataType dataType = DataType::UNKNOWN;

protected:
    // Nodes implement this to perform dynamic dispatch; returns a type-erased result.
    virtual std::any accept(ASTVisitorBase& visitor) const = 0;
};

inline std::string dataTypeToString(DataType type) {
    switch (type)
    {

    case DataType::INT  : return "Integer";
    case DataType::FLOAT    : return "Float";
    case DataType::BOOL     : return "Bool";
    case DataType::STRING   : return "String";
    case DataType::UNKNOWN  : return "Unknown";
    default                 : return "[UNKNOWN]";

    }
}



// ===== PODKLASY =====


/*
    struct CommandData {
        Token cmd;
        std::vector<ASTNode> args;
    };
*/
class CommandNode : public ASTNode {
public:
    Token command;
    std::vector<std::unique_ptr<ASTNode>> args;
    
    CommandNode(Token cmd, std::vector<std::unique_ptr<ASTNode>> args = {})
        : command(cmd), args(std::move(args)) {}

    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitCommand(*this);
    }
};


/*
    struct VarDeclData {
        Token name;
        std::unique_ptr<ASTNode> value;
        //DataType type;
    };
*/
class VarDeclNode : public ASTNode {
public:
    Token name;
    std::unique_ptr<ASTNode> value;
    
    VarDeclNode(Token name, DataType type, std::unique_ptr<ASTNode> value)
        : name(name), value(std::move(value)) {
        this->dataType = type;
    }
    
    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitVarDecl(*this);
    }
};


/*
    struct ExprData {
        Token token;
        //DataType type;
    };
*/
class ExprNode : public ASTNode {
public:
    Token token;
    
    ExprNode(Token token, DataType type)
        : token(token) {
        this->dataType = type;
    }
    
    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitExpr(*this);
    }
};


/*
    struct BinaryOpData {
        Token op;
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;
        //DataType type;
    };
*/
class BinaryOpNode : public ASTNode {
public:
    Token op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    
    BinaryOpNode(Token op, DataType type, std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right)
        : op(op), left(std::move(left)), right(std::move(right)) {
            this->dataType = type;
        }
    
    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitBinaryOp(*this);
    }
};


/*
    struct IfStmtData {
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> thenBranch;  // zawsze musi być
        std::unique_ptr<ASTNode> elseBranch;  // może być nullptr
    };
*/
class IfNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;  // can be nullptr
    
    IfNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> thenBranch, std::unique_ptr<ASTNode> elseBranch)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    
    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitIf(*this);
    }
};


/*
    struct WhileLoopData {
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> body;
    };
*/
class WhileNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
    
    WhileNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body)
        : condition(std::move(condition)), body(std::move(body)) {}
    
    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitWhile(*this);
    }
};


/*
    struct ScopeData {
        std::vector<ASTNode> statements;
    };  
*/
class ScopeNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    
    ScopeNode(std::vector<std::unique_ptr<ASTNode>> statements = {})
        : statements(std::move(statements)) {}
    
    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitScope(*this);
    }
};