// ast.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <any>

#include "tokenization.hpp" // Just for Token struct
#include "visitor.hpp"


enum class DataType {
    INT, FLOAT, BOOL, STRING, UNKNOWN
};

enum class VarStorageType {
    STORAGE, SCOREBOARD
};



struct VarInfo {
    std::string name;
    // --- Semantic Data ---
    DataType dataType;
    //int scopeLevel;      // Scope depth when the variable was initialized
    bool isConstant;
    std::string constValue;

    // --- Minecraft Data (Backend) ---
    VarStorageType storageType;
    std::string storageIdent;  
    std::string storagePath;
    
    // --- Additional Flags ---
    bool isUsed;
    bool isInitialized;
};


// =====  HELPER FUNCTIONS =====
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


// ========== CLASSES ==========
class ASTNode {
public:
    virtual ~ASTNode() = default;

    template<typename T>
    T visit(ASTVisitor<T>& visitor) const {
        return std::any_cast<T>(accept(visitor));
    }

    // Overload for void-result visitors so we don't attempt std::any_cast<void>.
    void visit(ASTVisitor<void>& visitor) const {
        accept(visitor);
    }

    // --- Semantic Data (Adnotations) ---
    mutable bool isAnalyzed = false;
    //mutable DataType dataType = DataType::UNKNOWN;

protected:
    // Nodes implement this to perform dynamic dispatch; returns a type-erased result.
    virtual std::any accept(ASTVisitorBase& visitor) const = 0;
};



// ===== SUBCLASSES =====

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


class VarDeclNode : public ASTNode {
public:
    Token name;
    std::unique_ptr<ASTNode> value;
 
    mutable std::shared_ptr<VarInfo> varInfo;
    
    VarDeclNode(Token name, std::unique_ptr<ASTNode> value)
        : name(name), value(std::move(value)) {}
    
    std::any accept(ASTVisitorBase& visitor) const override {
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
    
    std::any accept(ASTVisitorBase& visitor) const override {
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
    
    std::any accept(ASTVisitorBase& visitor) const override {
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
    
    std::any accept(ASTVisitorBase& visitor) const override {
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
    
    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitWhile(*this);
    }
};


class ScopeNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    
    ScopeNode(std::vector<std::unique_ptr<ASTNode>> statements = {})
        : statements(std::move(statements)) {}
    
    std::any accept(ASTVisitorBase& visitor) const override {
        return visitor.visitScope(*this);
    }
};