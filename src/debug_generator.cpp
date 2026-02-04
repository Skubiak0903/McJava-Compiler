// debug_generator.cpp
#include "./ast.hpp"

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
            arg->visit(*this);
        }
        indent_--;

        output_ << "\n";
    }
    
    void visitVarDeclT(const VarDeclNode& node) override {
        indent();
        
        if (node.isAnalyzed) {
            std::string name = node.varInfo->name;
            std::string type = ", Type: " + dataTypeToString(node.varInfo->dataType);
            std::string used = node.varInfo->isUsed ? ", [USED]" : ", [UNUSED]";
            std::string isConst = node.varInfo->isConstant ? ", [CONST: " + node.varInfo->constValue + "]" : ", [NON-CONST]";
            output_ << "VarDecl: " << name << type << used << isConst << "\n";
        } else {
            output_ << "VarDecl: " << node.name.value.value_or("[no name]") << "\n";
        }

        indent_++;
        node.value->visit(*this);
        indent_--; 

        output_ << "\n";
    }

    void visitExprT(const ExprNode& node) override {
        indent();
        if (node.isAnalyzed) {
            std::string value = node.token.value.value();
            std::string tokenType = " [" + tokenTypeToString(node.token.type) + "]"; 
            std::string type = ", Type: " + dataTypeToString(node.varInfo->dataType);
            std::string isConst = (node.varInfo->isConstant && !node.forceDynamic) ? ", [CONST: " + node.varInfo->constValue + "]" : ", [NON-CONST]";

            output_ << "Expr: " << value << tokenType << type << isConst << "\n";
        } else {
            output_ << "Expr: " << node.token.value.value_or("[no value]") 
                << " [" << tokenTypeToString(node.token.type) << "], "
                << "\n";
        }
    }

    void visitBinaryOpT(const BinaryOpNode& node) override {
        indent();
        if (node.isAnalyzed) {
            std::string value = node.op.value.value();
            std::string tokenType = " [" + tokenTypeToString(node.op.type) + "]"; 
            std::string type = ", Type: " + dataTypeToString(node.varInfo->dataType);
            std::string isConst = node.varInfo->isConstant ? ", [CONST: " + node.varInfo->constValue + "]" : ", [NON-CONST]";

            output_ << "BinaryOp: " << value << tokenType << type << isConst << "\n";
        } else {
            output_ << "BinaryOp: " << node.op.value.value_or("[no op]") 
                << " [" << tokenTypeToString(node.op.type) << "]," << "\n";
        }

        indent_++;
        node.left->visit(*this);
        node.right->visit(*this);
        indent_--; 
    }

    void visitIfT(const IfNode& node) override {
        indent();
        output_ << "IfStmt\n";

        indent_++;
        node.condition->visit(*this);
        node.thenBranch->visit(*this);
        indent_--;

        if (node.elseBranch) {
            indent();
            output_ << "else:\n";

            indent_++;
            node.elseBranch->visit(*this);
            indent_--;
        }

        output_ << "\n";
    }

    void visitWhileT(const WhileNode& node) override {
        indent();
        output_ << "WhileLoop\n";

        indent_++;
        node.condition->visit(*this);
        node.body->visit(*this);
        indent_--;

        output_ << "\n";
    }

    void visitScopeT(const ScopeNode& node) override {
        indent();
        output_ << "Scope {\n";
        
        indent_++;
        for (const auto& stmt : node.statements) {
            stmt->visit(*this);
        }
        indent_--;

        indent();
        output_ << "}\n";

        output_ << "\n";
    }
};