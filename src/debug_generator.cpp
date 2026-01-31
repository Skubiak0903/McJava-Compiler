// debug_generator.cpp

#include "./ast.hpp"

// VarInfo is unused here

class DebugGenerator : public ASTVisitor<void> {
private:
    std::ostream& output_;

    int indent_ = 0;
    
    void indent() {
        for (int i = 0; i < indent_; ++i) output_ << "  ";
    }

public:
    DebugGenerator(std::ostream& out) : output_(out) {}

    void visitCommandT(const CommandNode& node) override {
        indent();
        output_ << "Command: " << node.command.value.value_or("[no cmd]") << "\n";

        indent_++;
        for (const auto& arg : node.args) {
            arg->generate(*this);
        }
        indent_--;

        output_ << "\n";
    }
    
    void visitVarDeclT(const VarDeclNode& node) override {
        indent();
        output_ << "VarDecl: " << node.name.value.value_or("[no name]") << ", Type: " << dataTypeToString(node.dataType) << "\n";

        indent_++;
        node.value->generate(*this);
        indent_--; 

        output_ << "\n";
    }

    void visitExprT(const ExprNode& node) override {
        indent();
        output_ << "Expr: " << node.token.value.value_or("[no value]") 
            << " [" << tokenTypeToString(node.token.type) << "], "
            << "Type: " << dataTypeToString(node.dataType) << "\n";
    }

    void visitBinaryOpT(const BinaryOpNode& node) override {
        indent();
        output_ << "BinaryOp: " << node.op.value.value_or("[no op]") 
            << " [" << tokenTypeToString(node.op.type) << "],"
            << "Type: " << dataTypeToString(node.dataType) << "\n";
        
        indent_++;
        node.left->generate(*this);
        node.right->generate(*this);
        indent_--; 
    }

    void visitIfT(const IfNode& node) override {
        indent();
        output_ << "IfStmt\n";

        indent_++;
        node.condition->generate(*this);
        node.thenBranch->generate(*this);
        indent_--;

        if (node.elseBranch) {
            indent();
            output_ << "else:\n";

            indent_++;
            node.elseBranch->generate(*this);
            indent_--;
        }

        output_ << "\n";
    }

    void visitWhileT(const WhileNode& node) override {
        indent();
        output_ << "WhileLoop\n";

        indent_++;
        node.condition->generate(*this);
        node.body->generate(*this);
        indent_--;

        output_ << "\n";
    }

    void visitScopeT(const ScopeNode& node) override {
        indent();
        output_ << "Scope {\n";
        
        indent_++;
        for (const auto& stmt : node.statements) {
            stmt->generate(*this);
        }
        indent_--;

        indent();
        output_ << "}\n";

        output_ << "\n";
    }
};