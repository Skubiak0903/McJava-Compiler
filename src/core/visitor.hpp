// core/visitor.hpp
#pragma once

#include <variant>
#include <memory>

// Forward declarations
class CommandNode;
class VarDeclNode;
class ExprNode;
class BinaryOpNode;
class IfNode;
class WhileNode;
class ScopeNode;

struct VarInfo;
using ASTReturn = std::variant<std::monostate, std::shared_ptr<VarInfo>>;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual ASTReturn visitCommand  (const CommandNode& node)   = 0;
    virtual ASTReturn visitVarDecl  (const VarDeclNode& node)   = 0;
    virtual ASTReturn visitExpr     (const ExprNode& node)      = 0;
    virtual ASTReturn visitBinaryOp (const BinaryOpNode& node)  = 0;
    virtual ASTReturn visitIf       (const IfNode& node)        = 0;
    virtual ASTReturn visitWhile    (const WhileNode& node)     = 0;
    virtual ASTReturn visitScope    (const ScopeNode& node)     = 0;
};