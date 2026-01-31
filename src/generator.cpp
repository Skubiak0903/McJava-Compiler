// generator.cpp
#include <unordered_map>
#include <set>
#include <filesystem>

#include "./ast.hpp"

namespace fs = std::filesystem;


enum class VarStorageType {
    STORAGE, SCOREBOARD
};

// its not used in all visitors
struct VarInfo {
    DataType dataType;
    VarStorageType storageType;
    std::string storageIdent;
    std::string storagePath;
};

struct Scope {
    std::string name;
    fs::path path;
    std::unique_ptr<std::stringstream> output;
};



class FunctionGenerator : public ASTVisitor<std::shared_ptr<VarInfo>> {
private:
    fs::path& path_;
    std::string dp_prefix;
    std::string dp_path;

    std::unordered_map<std::string, VarInfo> variables_;
    std::vector<Scope> scopes_;
    size_t scopesTotalCount = 0;
    size_t tempVarCount = 0;

    std::string getCurrentScoreboard() {
        // std::string scopeName = getCurrentScope().name;
        std::string scopeName = "scope_0";
        return "mcjava_sb_" + scopeName;
    }

    std::string getTempVarName() {
        return "%" + std::to_string(tempVarCount++);
    }

    Scope& getCurrentScope() {
        return scopes_.back();
    }

    std::stringstream* getCurrentOutput() {
        return getCurrentScope().output.get();
    }

    void enterScope() {
        std::string name = "scope_" + std::to_string(scopesTotalCount++);
        fs::path path(path_ / (name + ".mcfunction"));
        auto output = std::make_unique<std::stringstream>();
        
        scopes_.push_back(Scope{
            name,
            path,
            std::move(output)
        });
    }
    
    void exitScope() {
        Scope& scope = getCurrentScope();
        
        // save to file
        std::ofstream file(scope.path, std::ios::out);
        if (!file.is_open()) error("Could not save function file!");

        // last scope (global)
        if (scopes_.size() == 1) {
            std::string body = scope.output->str();
            std::string header = prepareScoreboards();
            
            file << header << body;
        } else {
            file << scope.output->str();
        }

        file.close();
        scopes_.pop_back();
    }

    bool isComparisonOperator(TokenType type) const {
        return type == TokenType::LESS ||
        type == TokenType::GREATER ||
        type == TokenType::LESS_EQUAL ||
        type == TokenType::GREATER_EQUAL ||
        type == TokenType::EQUALS_EQUALS ||
        type == TokenType::NOT_EQUALS;
    }

    // std::fstream file(filename + ".mcfunction", std::ios::out);
    //     if (file.is_open()) {
    //         std::ostringstream codeBuffer;
    //         FunctionGenerator funcGen(codeBuffer);
            
    //         ast->generate(funcGen);
            
    //         file << funcGen.prepareScoreboards();
    //         file << codeBuffer.str();
            
    //     }
    //     file.close();

public:
    FunctionGenerator(fs::path& path, std::string dp_prefix, std::string dp_path) 
        : path_(path), dp_prefix(dp_prefix), dp_path(dp_path) {}

    std::shared_ptr<VarInfo> visitCommandT(const CommandNode& node) override {
        // only works for say
        std::string cmdKey = node.command.value.value();

        if (cmdKey != "say") error("Generator only supports 'say' command");
        
        std::ostringstream ss;
        ss << "tellraw @a [";

        for (const auto& arg : node.args) {
            if (arg->dataType == DataType::STRING) {
                ExprNode* exprNode = dynamic_cast<ExprNode*>(arg.get());
                if (!exprNode) continue;
                
                if (exprNode->token.type != TokenType::STRING_LIT) continue;
                
                ss << "{\"text\":\"" << exprNode->token.value.value() << "\"},";
                
                continue;
            }

            VarInfo tempVar = *arg->generate(*this);
            ss << "{\"score\":{\"name\":\"" << tempVar.storagePath << "\",\"objective\":\"" << tempVar.storageIdent << "\"}},";
        }
        ss << "]";

        auto output = getCurrentOutput();
        *output << ss.str() << "\n";

        return nullptr;
    }
//     void visit_command(const CommandData& cmd) override {
//         m_output << "# Command\n";
//         /*m_output << cmd.cmd.value.value_or("cmmd");

//         for (const auto& arg : cmd.args) {
//             m_output << " ";
//             apply_visitor(arg, *this);  // Rekurencyjnie generuj argumenty
//         }
//         m_output << "\n";*/
//     }
            
            
            
    std::shared_ptr<VarInfo> visitVarDeclT(const VarDeclNode& node) override {
        std::string varName = node.name.value.value();
        std::string currentSb = getCurrentScoreboard();
        VarInfo tempVar = *node.value->generate(*this);

        auto output = getCurrentOutput();

        *output << "#Debug: OPERATION 1 \n";
        *output << "scoreboard players operation " << varName << " " << currentSb << " = " << tempVar.storagePath << " " << tempVar.storageIdent << "\n";
        
        tempVar.dataType = node.dataType;
        tempVar.storagePath = varName;

        variables_[varName] = tempVar;
        return nullptr;
    }            

    std::shared_ptr<VarInfo> visitExprT(const ExprNode& node) override {
        // just assigns value to variable
        std::string currentSb = getCurrentScoreboard();
        std::string value = node.token.value.value();
        std::string varName = "%" + value;  // wont collide with any user defined variables

        auto output = getCurrentOutput();

        if (node.token.type == TokenType::IDENT) {
            // in this case => value is equal to variable name

            std::string varStorage = variables_[value].storageIdent;
            *output << "#Debug: OPERATION 2 \n";
            *output << "scoreboard players operation " << varName << " " << currentSb << " = " << value << " " << varStorage << "\n";
        } else {
            *output << "scoreboard players set " << varName << " " << currentSb << " " << value << "\n";
        }

        VarInfo varInfo = VarInfo{
                node.dataType,
                VarStorageType::SCOREBOARD,
                currentSb,
                varName
            };

        variables_[varName] = varInfo;
        return std::make_unique<VarInfo>(varInfo);
    }

    
    std::shared_ptr<VarInfo> visitBinaryOpT(const BinaryOpNode& node) override {
        // if binary op then repeat, if expression then it will add variable
        VarInfo leftVar  = *node.left ->generate(*this);
        VarInfo rightVar = *node.right->generate(*this);

        std::string tempVarName = getTempVarName();
        std::string currentSb = getCurrentScoreboard();
        auto output = getCurrentOutput();

        VarInfo varInfo = VarInfo{
                node.dataType,
                VarStorageType::SCOREBOARD,
                currentSb,
                tempVarName
            };

        variables_[tempVarName] = varInfo;

        switch (node.op.type) 
        {
        case TokenType::PLUS :
        case TokenType::MINUS :
        case TokenType::MULTIPLY :
        case TokenType::DIVIDE : {
            std::string comparator = node.op.value.value() + "=";
            
            // copy leftVar to not overwrite it
            *output << "#Debug: OPERATION 3 \n";
            *output << "scoreboard players operation " << tempVarName << " " << currentSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent  << "\n";

            *output << "#Debug: OPERATION 4 \n";
            *output << "scoreboard players operation " << tempVarName << " " << currentSb
                    << " " << comparator << " "       << rightVar.storagePath << " " << rightVar.storageIdent << "\n";

            return std::make_unique<VarInfo>(varInfo);
        }

        case TokenType::LESS :
        case TokenType::GREATER :
        case TokenType::LESS_EQUAL :
        case TokenType::GREATER_EQUAL : {
            std::string comparator = node.op.value.value();
                    
            *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " " << comparator << " " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            return std::make_unique<VarInfo>(varInfo);
        } 
        
        case TokenType::EQUALS_EQUALS : {
            *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            return std::make_unique<VarInfo>(varInfo);
        }
        case TokenType::NOT_EQUALS : {
            *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute unless score " << leftVar.storagePath << " " << leftVar.storageIdent
                    << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            return std::make_unique<VarInfo>(varInfo);
        }
    
        default:
            error("Unknown Token Type in binary operator");
        }

        return nullptr;

    }

    
    std::shared_ptr<VarInfo> visitIfT(const IfNode& node) override {
        if (node.elseBranch) {
            visitIfWithElse(node);
        } else {
            visitIfWithoutElse(node);
        }
        return nullptr;
    }

    void visitIfWithElse(const IfNode& node) {
        // schema:
        // if function 'then' returns 1 then run function 'else'
        // function then: return 1 unless condition is met

        // then scope
        enterScope();
        auto thenOutput = getCurrentOutput();
        std::string thenScopeName = getCurrentScope().name;
        VarInfo conditionVar = *node.condition->generate(*this);      

        // then branch
        *thenOutput << "# Then Body\n";

        *thenOutput << "execute unless score " << conditionVar.storagePath << " " << conditionVar.storageIdent << " matches 1 run return 1\n";
        auto thenScopeNode = dynamic_cast<ScopeNode*>(node.thenBranch.get());
        if (!thenScopeNode) {
            // single statement
            node.thenBranch->generate(*this);
        } else {
            // body is a scope
            for (const auto& stmt : thenScopeNode->statements) {
                stmt->generate(*this);
            }
        }
        exitScope();


        // else scope
        enterScope();
        auto elseOutput = getCurrentOutput();
        std::string elseScopeName = getCurrentScope().name;

        // else branch
        *elseOutput << "# Else Body\n";
        auto elseScopeNode = dynamic_cast<ScopeNode*>(node.elseBranch.get());
        if (!elseScopeNode) {
            // single statement
            node.elseBranch->generate(*this);
        } else {
            // body is a scope
            for (const auto& stmt : elseScopeNode->statements) {
                stmt->generate(*this);
            }
        }
        exitScope();

        auto mainOutput = getCurrentOutput();

        *mainOutput << "# Check condition  'if'\n";        
        // if thenScope branch returns 1 then execute else branch
        *mainOutput << "execute if function " << dp_prefix << ":" << dp_path << thenScopeName << " run function " << dp_prefix << ":" << dp_path << elseScopeName << "\n";

    }

    void visitIfWithoutElse(const IfNode& node) {
        // schema:
        // if condition then run function 'then'

        // then scope
        enterScope();
        auto thenOutput = getCurrentOutput();
        std::string thenScopeName = getCurrentScope().name;

        // then branch
        *thenOutput << "# Then Body\n";
        auto scopeNode = dynamic_cast<ScopeNode*>(node.thenBranch.get());
        if (!scopeNode) {
            // single statement
            node.thenBranch->generate(*this);
        } else {
            // body is a scope
            for (const auto& stmt : scopeNode->statements) {
                stmt->generate(*this);
            }
        }
        exitScope();

        auto mainOutput = getCurrentOutput();
        // first check to enter the loop
        *mainOutput << "# Check condition to enter the 'then' function\n";
        VarInfo conditionVar = *node.condition->generate(*this);        
        *mainOutput << "execute if score " << conditionVar.storagePath << " " << conditionVar.storageIdent << " matches 1 run function " << dp_prefix << ":" << dp_path << thenScopeName << "\n";
    }


    std::shared_ptr<VarInfo> visitWhileT(const WhileNode& node) override {
        // loop scope
        enterScope();
        auto whileOutput = getCurrentOutput();
        std::string scopeName = getCurrentScope().name;

        // loop body
        *whileOutput << "# Loop Body\n";
        auto scopeNode = dynamic_cast<ScopeNode*>(node.body.get());
        if (!scopeNode) {
            // single statement
            node.body->generate(*this);
        } else {
            // body is a scope
            for (const auto& stmt : scopeNode->statements) {
                stmt->generate(*this);
            }
        }

        // recheck condition at the end of the loop
        *whileOutput << "# Recheck condition at the end of the loop\n";
        VarInfo recheckVar = *node.condition->generate(*this);  
        *whileOutput << "execute if score " << recheckVar.storagePath << " " << recheckVar.storageIdent << " matches 1 run function " << dp_prefix << ":" << dp_path << scopeName << "\n";

        exitScope();

        auto mainOutput = getCurrentOutput();
        // first check to enter the loop
        *mainOutput << "# Check condition to enter the loop\n";
        VarInfo conditionVar = *node.condition->generate(*this);        
        *mainOutput << "execute if score " << conditionVar.storagePath << " " << conditionVar.storageIdent << " matches 1 run function " << dp_prefix << ":" << dp_path << scopeName << "\n";

        return nullptr;
    }



    std::shared_ptr<VarInfo> visitScopeT(const ScopeNode& node) override {
        enterScope();
                
        for (const auto& stmt : node.statements) {
            stmt->generate(*this);
        }
        
        exitScope();
        return nullptr;
    }

    std::string prepareScoreboards() {
        // Collect all unique scoreboard idents
        std::set<std::string> uniqueIdents;
        for (const auto& [name, var]: variables_) {
            uniqueIdents.insert(var.storageIdent);
        }
        
        std::ostringstream result;
        for (const auto& ident : uniqueIdents) {
            result << "scoreboard objectives add " << ident << " dummy\n";
        }

        return result.str();
    }

private:
    [[noreturn]] void error(const std::string& msg) {
        std::cerr << "Generation error: " << msg << std::endl;
        exit(EXIT_FAILURE);
    }

};