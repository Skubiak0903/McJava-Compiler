#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>

#include "./ast.hpp"

class Generator : public ASTVisitor {

struct Scope {
    std::unordered_set<std::string> variables;
    int tempCounter = 0;
};

/*struct ValueInfo {
    DataType type;
};*/

public:
    /*Generator() {
        //enterScope(); // enter global scope
    }

    ~Generator() {
        //exitScope(); // exit global scope after finished
    }*/

    std::string generate(const std::unique_ptr<ASTNode>& root) {
        m_output.str("");
        m_output.clear();
        
        // Użyj visitor pattern do generacji kodu
        //for (const ASTNode& node : root) {
            //apply_visitor(node, *this);
        //}

        apply_visitor(*root, *this);
        return m_output.str();
    }

    // Implementacje metod visitora
    void visit_command(const CommandData& cmd) override {
        m_output << "# Command\n";
        /*m_output << cmd.cmd.value.value_or("cmmd");
        
        for (const auto& arg : cmd.args) {
            m_output << " ";
            apply_visitor(arg, *this);  // Rekurencyjnie generuj argumenty
        }
        m_output << "\n";*/
    }

    void visit_vardecl(const VarDeclData& decl) override {
        if (!decl.value) {
            std::cerr << "Something happend while retriving declaration value";
            exit(EXIT_FAILURE);
        }

        std::string varName = decl.name.value.value();
        m_output << "## Var decl " << varName << "\n";

        // Zapisujemy zmienną w mapie
        Generator tempGen;
        apply_visitor(*(decl.value), tempGen);
        std::string variable = tempGen.getOutput();
        m_variables[varName] = DataType::BOOL;

        m_output << "# " << varName << " = " << variable << "\n";

        if (variable.empty()) {
            std::cerr << "Something happend while retriving declaration value";
            exit(EXIT_FAILURE);
        }


        // new line for easier debugging
        m_output << "\n";
    }
    
    void visit_expr(const ExprData& expr) override {
        std::string value = expr.token.value.value_or("");
        
        // Jeśli to identyfikator, sprawdź czy jest zmienną
        if (expr.token.type == TokenType::IDENT) {
            auto it = m_variables.find(value);
            if (it != m_variables.end()) {
                // m_output << it->second;
                return;
            }
        }
        
        //m_output << "\n# Expression\n";
        m_output << value;
    }
    
    //  aby zaimplementować użyj tak jak jest w vardecl bo używa on unique_ptr
    void visit_binary_op(const BinaryOpData& op) override {
        if (!op.op.value.has_value()) {
            std::cerr << "Something happend while retriving binary operator";
            exit(EXIT_FAILURE);
        }

        Generator tempGen1;
        apply_visitor(*op.left, tempGen1);
        std::string leftOutput = tempGen1.getOutput();

        Generator tempGen2;
        apply_visitor(*op.right, tempGen2);
        std::string rightOutput = tempGen2.getOutput();
        
        //m_output << "scoreboard objectives operations " << leftOutput << " " << op.op.value.value_or("dummy") << " " << rightOutput << "\n";*/
        m_output << "# Binary Operation: " << rightOutput << " " << op.op.value.value() << " " << leftOutput << "\n";
    }


    void visit_if_stmt(const IfStmtData& ifStmt) override {
        // Generuj warunek
        m_output << "# If Statement\n";

        /*m_output << "# If condition\n";
        apply_visitor(*ifStmt.condition, *this);

        
        // To uproszczone - w rzeczywistości potrzebujesz logiki scoreboard
        std::string tempVar = newTemp();
        m_output << "scoreboard objectives add " << tempVar << " dummy\n";
        m_output << "# Store condition result in " << tempVar << "\n";
        
        // Then branch
        m_output << "# Then branch\n";
        apply_visitor(*ifStmt.thenBranch, *this);
        
        // Else branch (jeśli istnieje)
        if (ifStmt.elseBranch) {
            m_output << "# Else branch\n";
            apply_visitor(*ifStmt.elseBranch, *this);
        }*/
    }

    void visit_while_loop(const WhileLoopData& whileLoop) override {
        m_output << "# While loop\n";
        /*std::string loopLabel = "__loop_" + std::to_string(rand());
        std::string endLabel = "__end_" + std::to_string(rand());
        
        m_output << "# While loop start\n";
        m_output << loopLabel << ":\n";
        
        // Warunek
        m_output << "# While condition\n";
        apply_visitor(*whileLoop.condition, *this);
        
        // To uproszczone - w Minecraft potrzebujesz execute if
        m_output << "# Jump if condition false\n";
        m_output << "# (execute unless score ... matches .. run goto " << endLabel << ")\n";
        
        // Ciało pętli
        m_output << "# While body\n";
        apply_visitor(*whileLoop.body, *this);
        
        // Skok do początku
        m_output << "# Jump to start\n";
        m_output << "goto " << loopLabel << "\n";
        
        m_output << endLabel << ":\n";
        m_output << "# End while\n";*/
    }

    void visit_scope(const ScopeData& scope) override {
        enterScope();

        for (const auto& stmt : scope.statements) {
            apply_visitor(stmt, *this);
        }

        exitScope();
    }
    
    
    std::string getOutput() const { return m_output.str(); }
    const auto& getVariables() const { return m_variables; }
    
    private:
    //const NodeProg& m_prog;
    std::stringstream m_output;
    // std::unordered_map<std::string, std::string> m_variables;
    std::unordered_map<std::string, DataType> m_variables;
    
    // Scope management
    std::vector<Scope> scopes;
    
    void enterScope() {
        m_output << "# Begin Scope\n";
        scopes.push_back(Scope{});
    }
    
    void exitScope() {
        m_output << "# Exit Scope\n";
        scopes.pop_back();
    }
    
    Scope& currentScope() {
        return scopes.back();
    }
    
    // Helper do generowania unikalnych nazw tymczasowych
    std::string newTemp() {
        auto& scope = currentScope();
        std::string name = "__temp" + std::to_string(scope.tempCounter++);
        scope.variables.insert(name);
        return name;
    }
};