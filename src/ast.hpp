// ast.hpp
#pragma once

#include <variant>
#include <vector>
#include <string>
#include "tokenization.hpp"

// ----------------------------------------------------
// NODE TYPES USING VARIANT + TEMPLATES
// ----------------------------------------------------

template<typename... Ts>
struct NodeVariant : std::variant<Ts...> {
    using std::variant<Ts...>::variant;
    
    template<typename Visitor>
    auto visit(Visitor&& vis) {
        return std::visit(std::forward<Visitor>(vis), *this);
    }
};

// Forward declarations for circular dependency
struct CommandData;
struct VarDeclData;
struct ExprData;
struct BinaryOpData;

// MAIN NODE TYPE - DODAJESZ TYLKO TU NOWE TYPY!
using ASTNode = NodeVariant<
    CommandData,
    VarDeclData,
    ExprData,
    BinaryOpData
    // DODAJ TU NOWE TYPY! np.:
    // IfStmtData,
    // WhileLoopData,
    // FunctionCallData
>;

// Concrete node types (now can use ASTNode)
struct CommandData {
    Token cmd;
    std::vector<ASTNode> args;
};

struct VarDeclData {
    Token name;
    std::unique_ptr<ASTNode> value;
};

struct ExprData {
    Token token;
};

struct BinaryOpData {
    Token op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
};

// ----------------------------------------------------
// EASY NODE CREATION
// ----------------------------------------------------

inline ASTNode make_command(Token cmd, std::vector<ASTNode> args = {}) {
    return CommandData{cmd, std::move(args)};
}

inline ASTNode make_vardecl(Token name, ASTNode value) {
    return VarDeclData{name, std::make_unique<ASTNode>(std::move(value))};
}

inline ASTNode make_expr(Token token) {
    return ExprData{token};
}

inline ASTNode make_binary_op(Token op, ASTNode left, ASTNode right) {
    return BinaryOpData{op, 
                        std::make_unique<ASTNode>(std::move(left)), 
                        std::make_unique<ASTNode>(std::move(right))};
}


// ----------------------------------------------------
// VISITOR TEMPLATE FOR GENERATION
// ----------------------------------------------------

class ASTVisitor {
public:
    virtual void visit_command(const CommandData& cmd) = 0;
    virtual void visit_vardecl(const VarDeclData& decl) = 0;
    virtual void visit_expr(const ExprData& expr) = 0;
    virtual void visit_binary_op(const BinaryOpData& op) = 0;
};

// Helper to apply visitor to node
inline void apply_visitor(const ASTNode& node, ASTVisitor& visitor) {
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        
        if constexpr (std::is_same_v<T, CommandData>) {
            visitor.visit_command(arg);
        } else if constexpr (std::is_same_v<T, VarDeclData>) {
            visitor.visit_vardecl(arg);
        } else if constexpr (std::is_same_v<T, ExprData>) {
            visitor.visit_expr(arg);
        } else if constexpr (std::is_same_v<T, BinaryOpData>) {
            visitor.visit_binary_op(arg);
        }
        // DODAJESZ TYLKO JEDEN IF_CONSTEXPR DLA NOWEGO TYPU!
    }, node);
}