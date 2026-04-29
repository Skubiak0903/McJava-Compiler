// backend/debug_generator.cpp
#include "./debug_generator.hpp"

#include "./../core/ast.hpp"

class DebugGenerator::Impl : public ASTVisitor {
private:
    std::ostream& output_;

    int indent_ = 0;
    
    void indent() {
        for (int i = 0; i < indent_; ++i) output_ << "  ";
    }

    void printAnnotations(const ASTNode& node) {
        if (node.annotations.empty()) return;
        
        indent();
        for (Annotation ann : node.annotations) {
            output_ << "@" << ann.name << ", ";
        }
        output_ << "\n";
    }

    inline ASTReturn done() { return std::monostate{}; }
    inline void visit(ASTNode& node) { node.accept(*this); }

public:
    Impl(std::ostream& out) : output_(out) {}

    void generate(ASTNode& node) {
        node.accept(*this);
    }

    ASTReturn visitCommand(const CommandNode& node) override {
        printAnnotations(node);

        indent();
        output_ << "Command: " << node.command.value.value_or("[no cmd]") << "\n";

        indent_++;
        for (const auto& arg : node.args) {
            visit(*arg);
        }
        indent_--;

        output_ << "\n";
        return done();
    }
    
    ASTReturn visitVarDecl(const VarDeclNode& node) override {
        printAnnotations(node);

        indent();
        
        if (node.isAnalyzed) {
            std::string name = node.varInfo->name;
            std::string type = ", Type: " + dataTypeToString(node.varInfo->dataType);
            std::string isConst = node.varInfo->isConstant ? ", [CONST: " + node.varInfo->constValue + "]" : ", [NON-CONST]";
            output_ << "VarDecl: " << name << type << isConst << "\n";
        } else {
            output_ << "VarDecl: " << node.name.value.value_or("[no name]") << "\n";
        }

        indent_++;
        visit(*node.value);
        indent_--; 

        output_ << "\n";
        return done();
    }

    ASTReturn visitVarAssign(const VarAssignNode& node) override {
        printAnnotations(node);

        indent();
        
        if (node.isAnalyzed) {
            std::string name = node.varInfo->name;
            std::string type = ", Type: " + dataTypeToString(node.varInfo->dataType);
            std::string used = node.varInfo->isUsed ? ", [USED]" : ", [UNUSED]";
            std::string isConst = node.varInfo->isConstant ? ", [CONST: " + node.varInfo->constValue + "]" : ", [NON-CONST]";
            std::string isExternal = node.varInfo->isExternal ? ", [EXTERNAL]" : ", [NON-EXTERNAL]";
            output_ << "VarAssign: " << name << type << used << isConst << isExternal << "\n";
        } else {
            output_ << "VarAssign: " << node.name.value.value_or("[no name]") << "\n";
        }

        indent_++;
        visit(*node.value);
        indent_--; 

        output_ << "\n";
        return done();
    }

    ASTReturn visitExpr(const ExprNode& node) override {
        printAnnotations(node);

        indent();
        if (node.isAnalyzed) {
            std::string value = node.token.value.value();
            std::string tokenType = " [" + tokenTypeToString(node.token.type) + "]"; 
            std::string type = ", Type: " + dataTypeToString(node.varInfo->dataType);
            std::string isConst = (node.varInfo->isConstant && !node.forceDynamic) ? ", [CONST: " + node.varInfo->constValue + "]" : ", [NON-CONST]";
            std::string scopeName = " [" + node.scope->name + "]";

            output_ << "Expr: " << value << tokenType << type << isConst << scopeName << "\n";
        } else {
            output_ << "Expr: " << node.token.value.value_or("[no value]") 
                << " [" << tokenTypeToString(node.token.type) << "], "
                << "\n";
        }
        return done();
    }

    ASTReturn visitBinaryOp(const BinaryOpNode& node) override {
        printAnnotations(node);

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
        visit(*node.left);
        visit(*node.right);
        indent_--; 

        return done();
    }

    ASTReturn visitIf(const IfNode& node) override {
        printAnnotations(node);
        
        indent();
        std::string isConst = node.isConditionConstant ? (" [CONST: " + std::to_string(node.conditionValue) + "]") : " [NON-CONST]";
        std::string scopeName = node.scope ? " [" + node.scope->name + "]" : "";
        output_ << "IfStmt" << isConst << scopeName << "\n";
        
        indent_++;
        visit(*node.condition);
        visit(*node.thenBranch);
        indent_--;

        if (node.elseBranch) {
            indent();
            output_ << "else:\n";

            indent_++;
            visit(*node.elseBranch);
            indent_--;
        }
        
        output_ << "\n";
        return done();
    }

    ASTReturn visitWhile(const WhileNode& node) override {
        printAnnotations(node);

        indent();
        std::string isConst = node.isConditionConstant ? (" [CONST: " + std::to_string(node.conditionValue) + "]") : " [NON-CONST]";
        output_ << "WhileLoop" << isConst << "\n";
        
        indent_++;
        visit(*node.condition);
        visit(*node.body);
        indent_--;
        
        output_ << "\n";
        return done();
    }
    
    ASTReturn visitScope(const ScopeNode& node) override {
        printAnnotations(node);
        
        indent();
        std::string scopeName = node.scope ? (" [Parent: " + node.scope->name + "]") : "";
        output_ << "Scope " << scopeName << " {\n";
        
        indent_++;
        for (const auto& stmt : node.statements) {
            visit(*stmt);
        }
        indent_--;

        indent();
        output_ << "}\n";

        output_ << "\n";
        return done();
    }

    ASTReturn visitFuncDecl(const FuncDeclNode& node) override {
        printAnnotations(node);

        std::string name = node.name.value.value();

        indent();
        output_ << "FuncDecl " << name << "() {\n";

        indent_++;
        visit(*node.body);
        indent_--;

        indent();
        output_ << "}\n";

        output_ << "\n";

        return done();
    }

    ASTReturn visitFuncCall(const FuncCallNode& node) override {
        printAnnotations(node);

        std::string name = node.name.value.value();
        
        indent();
        output_ << "FuncCall " << name << "()\n";

        output_ << "\n";

        return done();
    }

};

// ========== WRAPPER ==========
DebugGenerator::DebugGenerator(std::ostream& out)
    : pImpl(std::make_unique<Impl>(out)) {}

DebugGenerator::~DebugGenerator() = default; // Needed for unique_ptr<Impl>

void DebugGenerator::generate(ASTNode& node) {
    pImpl->generate(node);
}