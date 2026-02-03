// generator.cpp
#include <unordered_map>
#include <set>
#include <filesystem>
#include <optional>

#include "./ast.hpp"

namespace fs = std::filesystem;

struct Scope {
    std::string name;
    fs::path path;
    std::unique_ptr<std::stringstream> output;
};

struct SimpleVarInfo {
    //DataType dataType;
    //int scopeLevel;      // Scope depth when the variable was initialized

    // --- Minecraft Data (Backend) ---
    VarStorageType storageType;
    std::string storageIdent;  
    std::string storagePath;
    
    // --- Constant Data ----
    bool isConstant;
    std::string constValue;
};



class FunctionGenerator : public ASTVisitor<std::shared_ptr<SimpleVarInfo>> {
private:
    const fs::path& path_;
    const std::string dp_prefix;
    const std::string dp_path;

    std::unordered_map<std::string, SimpleVarInfo> variables_;
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

public:
    FunctionGenerator(fs::path& path, std::string dp_prefix, std::string dp_path, std::unordered_map<std::string, std::shared_ptr<VarInfo>> variables) 
        : path_(path), dp_prefix(dp_prefix), dp_path(dp_path)  {

            std::string currentSb = getCurrentScoreboard();
            // convert variables
            for (const auto& [name, varInfo] : variables) {
                SimpleVarInfo simpleVar{
                    .storageType = VarStorageType::SCOREBOARD,
                    .storageIdent = currentSb,
                    .storagePath = name,
                    .isConstant = varInfo->isConstant,
                    .constValue = varInfo->constValue
                };
                variables_.emplace(name, simpleVar);
            }
        }


    std::shared_ptr<SimpleVarInfo> visitCommandT(const CommandNode& node) override {
        generateCommand(node);
        return nullptr;
    }

    std::shared_ptr<SimpleVarInfo> visitVarDeclT(const VarDeclNode& node) override {
        generateVarDecl(node);
        return nullptr;
    }

    std::shared_ptr<SimpleVarInfo> visitExprT(const ExprNode& node) override {
        return generateExpr(node); 
    }

    std::shared_ptr<SimpleVarInfo> visitBinaryOpT(const BinaryOpNode& node) override {
        return generateBinaryOp(node);
    }

    std::shared_ptr<SimpleVarInfo> visitIfT(const IfNode& node) override {
        if (node.elseBranch) {
            generateIfWithElse(node);
        } else {
            generateOnlyIf(node);
        }
        return nullptr;
    }

    std::shared_ptr<SimpleVarInfo> visitWhileT(const WhileNode& node) override {
        generateWhile(node);
        return nullptr;
    }

    std::shared_ptr<SimpleVarInfo> visitScopeT(const ScopeNode& node) override {
        generateScope(node);
        return nullptr;
    }



private:
    void generateCommand(const CommandNode& node) {
        // only works for say
        std::string cmdKey = node.command.value.value();

        if (cmdKey != "say") error("Generator only supports 'say' command");
        
        std::ostringstream ss;
        ss << "tellraw @a [";

        for (const auto& arg : node.args) {
            ExprNode* exprNode = dynamic_cast<ExprNode*>(arg.get());
            if (exprNode) {                
                if (exprNode->token.type != TokenType::STRING_LIT) continue;
                
                ss << "{\"text\":\"" << exprNode->token.value.value() << "\"},";
                
                continue;
            }

            SimpleVarInfo tempVar = *arg->visit(*this);
            ss << "{\"score\":{\"name\":\"" << tempVar.storagePath << "\",\"objective\":\"" << tempVar.storageIdent << "\"}},";
        }
        ss << "]";

        auto output = getCurrentOutput();
        *output << ss.str() << "\n";
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
            
            
            
    void generateVarDecl(const VarDeclNode& node) {
        std::string varName = node.name.value.value();
        std::string currentSb = getCurrentScoreboard();
        auto output = getCurrentOutput();

        /*auto exprNode = dynamic_cast<ExprNode*>(node.value.get());
        if (exprNode && exprNode->token.type != TokenType::IDENT) {
            // its constant

            *output << "#Debug: Constant var\n";
            *output << "scoreboard players set " << varName << " " << currentSb << " " << exprNode->token.value.value() << "\n";

            VarInfo varInfo = VarInfo{
                node.dataType,
                VarStorageType::SCOREBOARD,
                currentSb,
                varName,
                // even if its another declaration the value will be still constant,
                // because we declare it with constant value
                // ex. x = 20
                true,
                exprNode->token.value.value()
            };
            variables_[varName] = varInfo;
            return;
        }
        
        VarInfo tempVar = *node.value->visit(*this);
        *output << "#Debug: OPERATION 1 \n";
        *output << "scoreboard players operation " << varName << " " << currentSb << " = " << tempVar.storagePath << " " << tempVar.storageIdent << "\n";
        
        tempVar.dataType = node.dataType;
        tempVar.storagePath = varName;

        variables_[varName] = tempVar;*/

        if (node.varInfo->isConstant) {
            *output << "#Debug: Constant var\n";
            *output << "scoreboard players set " << varName << " " << currentSb << " " << node.varInfo->constValue << "\n";
        } else {
            SimpleVarInfo tempVar = *node.value->visit(*this);

            *output << "#Debug: Dynamic var \n";
            *output << "scoreboard players operation " << varName << " " << currentSb << " = " << tempVar.storagePath << " " << tempVar.storageIdent << "\n";
        }
    }


    std::shared_ptr<SimpleVarInfo> generateExpr(const ExprNode& node) {
        // just assigns value to variable
        std::string currentSb = getCurrentScoreboard();
        std::string tokValue = node.token.value.value();
        std::string varName = "%" +  tokValue; // wont collide with any user defined variables

        auto output = getCurrentOutput();

        SimpleVarInfo varInfo = SimpleVarInfo{
            .storageType = VarStorageType::SCOREBOARD,
            .storageIdent = currentSb,
            .storagePath = varName,
            .isConstant = node.varInfo->isConstant,
            .constValue = node.varInfo->constValue,
        };

        // if constant -> dont generate, higher node should catch it properly
        if(node.varInfo->isConstant) {
            varInfo.storagePath = tokValue;
            return std::make_shared<SimpleVarInfo>(varInfo);
        }

        // if (node.token.type == TokenType::IDENT) {
        //     // in this case => value is equal to variable name
            
        //     std::string varStorage = variables_[value].storageIdent;
        //     *output << "#Debug: OPERATION 2 \n";
        //     *output << "scoreboard players operation " << varName << " " << currentSb << " = " << value << " " << varStorage << "\n";
            
        //     varInfo.isConstant = false;
        // } else {
        //     *output << "scoreboard players set " << varName << " " << currentSb << " " << value << "\n";
        //     varInfo.isConstant = true;
        //     varInfo.value = value;
        // }

        if (node.varInfo->isConstant) {
            *output << "#Debug: Constant Expression\n";
            *output << "scoreboard players set " << varName << " " << currentSb << " " << node.varInfo->constValue << "\n";
        } else {
            // retrive variable
            SimpleVarInfo varInfo = variables_.at(node.token.value.value());

            *output << "#Debug: Dynamic Expression\n";
            *output << "scoreboard players operation " << varName << " " << currentSb << " = " << varInfo.storagePath << " " << varInfo.storageIdent << "\n";
        }

        //variables_[varName] = varInfo;
        return std::make_unique<SimpleVarInfo>(varInfo);
    }

    
    std::shared_ptr<SimpleVarInfo> generateBinaryOp(const BinaryOpNode& node) {
        std::string currentSb = getCurrentScoreboard();
        auto output = getCurrentOutput();

        // we can generate these 2 nodes because there is at least 1 variable -> analyzer combined all 2 constants binary operators
        SimpleVarInfo leftVar  = *node.left  ->visit(*this);
        SimpleVarInfo rightVar = *node.right ->visit(*this);
        
        /*auto rightVarNode = dynamic_cast<ExprNode*>(node.right.get());
        VarInfo* rightVar = nullptr;
        if (rightVarNode == nullptr) {
            // If right is not an ExprNode, generate it as a regular node
            VarInfo rightVarInfo = *node.right->visit(*this);
            rightVar = &rightVarInfo;
        }*/

        std::string tempVarName = getTempVarName();

        SimpleVarInfo varInfo = {
            .storageType  = VarStorageType::SCOREBOARD,
            .storageIdent = currentSb,
            .storagePath  = tempVarName,
            .isConstant   = false // there is at least 1 variable in this binary operation
        };

        //variables_[tempVarName] = varInfo;

        switch (node.op.type) 
        {
        case TokenType::PLUS : {
            // THERE IS NO WAY TO OPTIMIZE THIS FURTHER DUE TO MINECRAFT COMMAND LIMITATIONS
            // 2 commands is the minimum for addition and there isn't any gain in performace

            /*if (rightVarNode != nullptr && rightVarNode->token.type != TokenType::IDENT) {
                *output << "#Debug: Constant Value With Addition \n";
                *output << "scoreboard players add " << tempVarName << " " << currentSb << " " << rightVarNode->token.value.value() << "\n";
                return std::make_unique<VarInfo>(varInfo);
            }
            [[fallthrough]];*/

            /*if (rightVar.isConstant) {
                // Optimalized: scoreboard players add
                *output << "scoreboard players set " << tempVarName << " " << currentSb << " 0\n"; // Initialization not needed
                *output << "#Debug: Scoreboard ADD -> rightVar is constant";
                *output << "scoreboard players operation " << tempVarName << " " << currentSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent << "\n";
                *output << "scoreboard players add " << tempVarName << " " << currentSb << " " << rightVar.constValue << "\n";
            } else {
                // Standard operation: scoreboard players operation +=
                *output << "#Debug: Default binary op -> rightVar isn't constant";
                *output << "scoreboard players operation " << tempVarName << " " << currentSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent << "\n";
                *output << "scoreboard players operation " << tempVarName << " " << currentSb << " += " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            }*/

            [[fallthrough]];
        }
        case TokenType::MINUS : 
        case TokenType::MULTIPLY :
        case TokenType::DIVIDE : {
            std::string comparator = node.op.value.value() + "=";

            /*if (rightVarNode != nullptr) {
                VarInfo rightVarInfo = *node.right->visit(*this);
                rightVar = &rightVarInfo;
            }
            
            // copy leftVar to not overwrite it
            *output << "#Debug: OPERATION 3 \n";
            *output << "scoreboard players operation " << tempVarName << " " << currentSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent  << "\n";

            *output << "#Debug: OPERATION 4 \n";
            *output << "scoreboard players operation " << tempVarName << " " << currentSb
                    << " " << comparator << " "       << rightVar->storagePath << " " << rightVar->storageIdent << "\n";

            return std::make_unique<VarInfo>(varInfo);*/

            // constants doesnt help with performance in this case
            *output << "#DEBUG: BinaryOp -> Arithmetic operation\n";
            *output << "scoreboard players operation " << tempVarName << " " << currentSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent  << "\n";
            *output << "scoreboard players operation " << tempVarName << " " << currentSb
                    << " " << comparator << " "       << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            
            break;
        }

        case TokenType::LESS :
        case TokenType::GREATER :
        case TokenType::LESS_EQUAL :
        case TokenType::GREATER_EQUAL : {
            std::string comparator = node.op.value.value();

            /*if (rightVarNode != nullptr) {
                int value = std::stoi(rightVarNode->token.value.value());

                if      (comparator == ">") value += 1;
                else if (comparator == "<") value -= 1;

                std::string sValue;
                if      (comparator == ">" || comparator == ">=") sValue = std::to_string(value) + ".."; 
                else if (comparator == "<" || comparator == "<=") sValue = ".." + std::to_string(value); 

                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << sValue << "\n";
                return std::make_unique<VarInfo>(varInfo);
            }*/
           
            if (rightVar.isConstant) {
                int value = std::stoi(rightVar.constValue);

                // x > 1   ->  matches 2..
                // x < 1   ->  matches ..0
                // x >= 1  ->  matches 1..
                // x <= 1  ->  matches ..1

                if      (comparator == ">") value += 1;
                else if (comparator == "<") value -= 1;

                std::string sValue;
                if      (comparator == ">" || comparator == ">=") sValue = std::to_string(value) + ".."; 
                else if (comparator == "<" || comparator == "<=") sValue = ".." + std::to_string(value); 

                *output << "#DEBUG: BinaryOp -> Comparition operation -> RightVar is constant\n";
                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << sValue << "\n";
                
                break;
            }

            if (leftVar.isConstant) {
                // flip comparator & vars
                int value = std::stoi(leftVar.constValue);

                // 1 > x   ->  x < 1   ->  matches ..0
                // 1 < x   ->  x > 1   ->  matches 2..
                // 1 >= x  ->  x <= 1  ->  matches ..1
                // 1 <= x  ->  x >= 1  ->  matches 1..

                // flipped logic
                if      (comparator == ">") value -= 1;
                else if (comparator == "<") value += 1;

                std::string sValue;
                if      (comparator == ">" || comparator == ">=") sValue = ".." + std::to_string(value); 
                else if (comparator == "<" || comparator == "<=") sValue = std::to_string(value) + ".."; 

                *output << "#DEBUG: BinaryOp -> Comparison operation -> LeftVar is constant\n";
                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << rightVar.storagePath << " " << rightVar.storageIdent
                    << " matches " << sValue << "\n";
                
                break;
            }

            *output << "#DEBUG: BinaryOp -> Default Comparison operation\n";
            *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " " << comparator << " " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            //return std::make_unique<VarInfo>(varInfo);

            break;
        } 
        
        case TokenType::EQUALS_EQUALS : {
            if (rightVar.isConstant) {
                std::string value = rightVar.constValue;

                *output << "#DEBUG: BinaryOp -> Equals Comparison operation -> RightVar is const\n";
                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            if (leftVar.isConstant) {
                std::string value = leftVar.constValue;

                *output << "#DEBUG: BinaryOp -> Equals Comparison operation -> LeftVar is const\n";
                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << rightVar.storagePath << " " << rightVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            /*if (rightVarNode != nullptr) {
                std::string value = rightVarNode->token.value.value();

                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << value << "\n";
                return std::make_unique<VarInfo>(varInfo);
            }*/

            *output << "#DEBUG: BinaryOp -> Default Equals Comparison operation\n";
            *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            //return std::make_unique<VarInfo>(varInfo);
            break;
        }
        case TokenType::NOT_EQUALS : {
            /*if (rightVarNode != nullptr) {
                std::string value = rightVarNode->token.value.value();

                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute unless score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << value << "\n";
                return std::make_unique<VarInfo>(varInfo);
            }*/

            if (rightVar.isConstant) {
                std::string value = rightVar.constValue;

                *output << "#DEBUG: BinaryOp -> Equals Comparison operation -> RightVar is const\n";
                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute unless score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            if (leftVar.isConstant) {
                std::string value = leftVar.constValue;

                *output << "#DEBUG: BinaryOp -> Equals Comparison operation -> LeftVar is const\n";
                *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute unless score " << rightVar.storagePath << " " << rightVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            *output << "execute store success score " << tempVarName << " " << currentSb 
                    << " run execute unless score " << leftVar.storagePath << " " << leftVar.storageIdent
                    << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            //return std::make_unique<VarInfo>(varInfo);

            break;
        }
    
        default:
            error("Unknown Token Type in binary operator");
        }

        return std::make_unique<SimpleVarInfo>(varInfo);
    }

    void generateIfWithElse(const IfNode& node) {
        // schema:
        // if function 'then' returns 1 then run function 'else'
        // function then: return 1 unless condition is met

        // then scope
        enterScope();
        auto thenOutput = getCurrentOutput();
        std::string thenScopeName = getCurrentScope().name;
        SimpleVarInfo conditionVar = *node.condition->visit(*this);      

        // then branch
        *thenOutput << "# Then Body\n";

        *thenOutput << "execute unless score " << conditionVar.storagePath << " " << conditionVar.storageIdent << " matches 1 run return 1\n";
        auto thenScopeNode = dynamic_cast<ScopeNode*>(node.thenBranch.get());
        if (!thenScopeNode) {
            // single statement
            node.thenBranch->visit(*this);
        } else {
            // body is a scope
            for (const auto& stmt : thenScopeNode->statements) {
                stmt->visit(*this);
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
            node.elseBranch->visit(*this);
        } else {
            // body is a scope
            for (const auto& stmt : elseScopeNode->statements) {
                stmt->visit(*this);
            }
        }
        exitScope();

        auto mainOutput = getCurrentOutput();

        *mainOutput << "# Check condition  'if'\n";        
        // if thenScope branch returns 1 then execute else branch
        *mainOutput << "execute if function " << dp_prefix << ":" << dp_path << thenScopeName << " run function " << dp_prefix << ":" << dp_path << elseScopeName << "\n";

    }

    void generateOnlyIf(const IfNode& node) {
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
            node.thenBranch->visit(*this);
        } else {
            // body is a scope
            for (const auto& stmt : scopeNode->statements) {
                stmt->visit(*this);
            }
        }
        exitScope();

        auto mainOutput = getCurrentOutput();
        // first check to enter the loop
        *mainOutput << "# Check condition to enter the 'then' function\n";
        SimpleVarInfo conditionVar = *node.condition->visit(*this);        
        *mainOutput << "execute if score " << conditionVar.storagePath << " " << conditionVar.storageIdent << " matches 1 run function " << dp_prefix << ":" << dp_path << thenScopeName << "\n";
    }


    void generateWhile(const WhileNode& node) {
        // loop scope
        enterScope();
        auto whileOutput = getCurrentOutput();
        std::string scopeName = getCurrentScope().name;

        // loop body
        *whileOutput << "# Loop Body\n";
        auto scopeNode = dynamic_cast<ScopeNode*>(node.body.get());
        if (!scopeNode) {
            // single statement
            node.body->visit(*this);
        } else {
            // body is a scope
            for (const auto& stmt : scopeNode->statements) {
                stmt->visit(*this);
            }
        }

        // recheck condition at the end of the loop
        *whileOutput << "# Recheck condition at the end of the loop\n";
        SimpleVarInfo recheckVar = *node.condition->visit(*this);  
        *whileOutput << "execute if score " << recheckVar.storagePath << " " << recheckVar.storageIdent << " matches 1 run function " << dp_prefix << ":" << dp_path << scopeName << "\n";

        exitScope();

        auto mainOutput = getCurrentOutput();
        // first check to enter the loop
        *mainOutput << "# Check condition to enter the loop\n";
        SimpleVarInfo conditionVar = *node.condition->visit(*this);        
        *mainOutput << "execute if score " << conditionVar.storagePath << " " << conditionVar.storageIdent << " matches 1 run function " << dp_prefix << ":" << dp_path << scopeName << "\n";
    }



    void generateScope(const ScopeNode& node) {
        enterScope();
                
        for (const auto& stmt : node.statements) {
            stmt->visit(*this);
        }
        
        exitScope();
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