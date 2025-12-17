#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "./ast.hpp"

class Generator : public ASTVisitor {
public:

    std::string generate(const std::vector<ASTNode>& root) {
        m_output.str("");
        m_output.clear();
        
        // Użyj visitor pattern do generacji kodu
        for (const ASTNode& node : root) {
            apply_visitor(node, *this);
        }
        return m_output.str();
    }

    // Implementacje metod visitora
    void visit_command(const CommandData& cmd) override {
        m_output << cmd.cmd.value.value_or("cmmd");
        
        for (const auto& arg : cmd.args) {
            m_output << " ";
            apply_visitor(arg, *this);  // Rekurencyjnie generuj argumenty
        }
        m_output << "\n";
    }

    void visit_vardecl(const VarDeclData& decl) override {
        std::string varName = decl.name.value.value_or("");
        
        // Zapisujemy zmienną w mapie
        if (decl.value) {
            std::stringstream ss;
            Generator tempGen;
            apply_visitor(*(decl.value), tempGen);
            m_variables[varName] = tempGen.getOutput();
        }
        
        // Generujemy komendę
        m_output << "# " << varName << " = ";
        if (decl.value) {
            apply_visitor(*(decl.value), *this);
        } else {
            m_output << "0";
        }
        m_output << "\n";
    }
    
    void visit_expr(const ExprData& expr) override {
        std::string value = expr.token.value.value_or("");
        
        // Jeśli to identyfikator, sprawdź czy jest zmienną
        if (expr.token.type == TokenType::IDENT) {
            auto it = m_variables.find(value);
            if (it != m_variables.end()) {
                m_output << it->second;
                return;
            }
        }
        
        m_output << value;
    }
    
    //  aby zaimplementować użyj tak jak jest w vardecl bo używa on unique_ptr
    void visit_binary_op(const BinaryOpData& op) override {
        // Obsługa operacji binarnych (na przyszłość)
        std::cerr << "Binary operations not yet implemented!\n";
        m_output << "0";  // placeholder
    }


    std::string getOutput() const { return m_output.str(); }
    const auto& getVariables() const { return m_variables; }

private:
    //const NodeProg& m_prog;
    std::stringstream m_output;
    std::unordered_map<std::string, std::string> m_variables;
};