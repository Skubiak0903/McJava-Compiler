// generator.cpp
#include <unordered_map>
#include <set>
#include <filesystem>
#include <optional>

#include "./ast.hpp"
#include "./options.hpp"

namespace fs = std::filesystem;

struct Scope {
    std::string name;
    fs::path path;
    std::unique_ptr<std::stringstream> output;
};


class FunctionGenerator : public ASTVisitor<std::shared_ptr<VarInfo>> {
private:
    const fs::path& path_;
    const Options& options_;

    std::unordered_map<std::string, std::shared_ptr<VarInfo>> variables_;
    std::vector<Scope> scopes_;
    size_t scopesTotalCount = 0;

    Scope& getCurrentScope() {
        return scopes_.back();
    }

    std::stringstream* getCurrentOutput() {
        return getCurrentScope().output.get();
    }

    std::string getFunctionNameSpace() {
        return options_.dpPrefix + ":" + options_.dpPath;
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
    FunctionGenerator(fs::path& path, Options& options, std::unordered_map<std::string, std::shared_ptr<VarInfo>> variables) 
        : path_(path), options_(options), variables_(std::move(variables)) {}


    std::shared_ptr<VarInfo> visitCommandT(const CommandNode& node) override {
        generateCommand(node);
        return nullptr;
    }

    std::shared_ptr<VarInfo> visitVarDeclT(const VarDeclNode& node) override {
        generateVarDecl(node);
        return nullptr;
    }

    std::shared_ptr<VarInfo> visitExprT(const ExprNode& node) override {
        return generateExpr(node); 
    }

    std::shared_ptr<VarInfo> visitBinaryOpT(const BinaryOpNode& node) override {
        return generateBinaryOp(node);
    }

    std::shared_ptr<VarInfo> visitIfT(const IfNode& node) override {
        if (node.elseBranch) {
            generateIfWithElse(node);
        } else {
            generateOnlyIf(node);
        }
        return nullptr;
    }

    std::shared_ptr<VarInfo> visitWhileT(const WhileNode& node) override {
        generateWhile(node);
        return nullptr;
    }

    std::shared_ptr<VarInfo> visitScopeT(const ScopeNode& node) override {
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
                if (exprNode->token.type == TokenType::STRING_LIT) {
                    ss << "{\"text\":\"" << exprNode->token.value.value() << "\"},";
                    continue;
                } else if (exprNode->varInfo->isConstant) {
                    ss << "{\"text\":\"" << exprNode->varInfo->constValue << "\"},";
                    continue;
                }
            }

            BinaryOpNode* binOpNode = dynamic_cast<BinaryOpNode*>(arg.get());
            if (binOpNode && binOpNode->varInfo->isConstant) {                
                ss << "{\"text\":\"" << binOpNode->varInfo->constValue << "\"},";
                continue;
            }

            VarInfo tempVar = *arg->visit(*this);
            ss << "{\"score\":{\"name\":\"" << tempVar.storagePath << "\",\"objective\":\"" << tempVar.storageIdent << "\"}},";
        }
        ss << "]";

        auto output = getCurrentOutput();
        *output << ss.str() << "\n";
    }
            
            
            
    void generateVarDecl(const VarDeclNode& node) {

        // dont emit unused variables
        if (!node.varInfo->isUsed && options_.removeUnusedVars) {
            return;
        }
        
        // if its used but its constant then also dont emit it
        // example:
        //   x = 10
        //   say x
        // 
        // it would be compiled to:
        //   scoreboard players set x mcjava_sb_scope_0 10
        //   tellraw @a [{"text":"10"},]
        // 
        // but we still arent using the x variable
        // 
        // NOTE: it doest work when expression folding is disabled
        if (node.varInfo->isConstant && node.varInfo->isUsed && options_.doConstantFolding) { // we dont need to add node.varInfo->isUsed -> all unused were remove above
            return;
        }
       
        std::string varName = node.varInfo->name;
        auto output = getCurrentOutput();
       
        if (node.varInfo->isConstant) {
            *output << "#Debug: Constant var\n";
            *output << "scoreboard players set " << varName << " " << node.varInfo->storageIdent << " " << node.varInfo->constValue << "\n";
        } else {
            VarInfo tempVar = *node.value->visit(*this);

            *output << "#Debug: Dynamic var \n";
            *output << "scoreboard players operation " << varName << " " << node.varInfo->storageIdent << " = " << tempVar.storagePath << " " << tempVar.storageIdent << "\n";
        }
    }



    // LEFT UNREFACTORED FOR NOW -> NOT SURE IF THIS IS THE 100% CORRECT 
    std::shared_ptr<VarInfo> generateExpr(const ExprNode& node) {
        // just assigns value to variable
        std::string tokValue = node.token.value.value(); // variable name in user code
        //std::string varName = "%" +  tokValue; // won't collide with any user defined variables

        //auto output = getCurrentOutput();

        // VarInfo varInfo = VarInfo{
        //     .storageType = VarStorageType::SCOREBOARD,
        //     .storageIdent = currentSb,
        //     .storagePath = varName,
        //     .isConstant = node.varInfo->isConstant,
        //     .constValue = node.varInfo->constValue,
        // };

        // if constant -> dont generate, higher node should implement it properly
        if(node.varInfo->isConstant && !node.forceDynamic) {
            
            // we dont want to change anything in variables -> just generate it
            //node.varInfo->storagePath = tokValue;
            //node.varInfo->storageIdent = getCurrentScoreboard();
            return node.varInfo;
        }

        // this block appears to be unreachable
        /*if (node.varInfo->isConstant) {
            *output << "#Debug: Constant Expression\n";
            *output << "scoreboard players set " << varName << " " << currentSb << " " << node.varInfo->constValue << "\n";
        } else */

        {
            // retrive variable -> variable exists because analyzer checked it
            auto varInfo = variables_.at(node.token.value.value());

            // can be wrong but we dont need to emits anything becouse we are only copying this value, and
            // the binary operation copy values that they change by themselves
            // and if we need to only copy the variable then we dont need to pass the real VarInfo highier 
            
            //*output << "#Debug: Dynamic Expression\n";
            //*output << "scoreboard players operation " << varName << " " << currentSb << " = " << varInfo.storagePath << " " << varInfo.storageIdent << "\n";

            return varInfo;
        }
    }


    /*
      TODO: Fix this

      case: 
        y = x + 5;
        x = y;
    
      generates to this:

        #DEBUG: BinaryOp -> Arithmetic operation
        scoreboard players operation %1 mcjava_sb_scope_0 = x mcjava_sb_scope_0
        scoreboard players operation %1 mcjava_sb_scope_0 += 5 mcjava_sb_scope_0

        #Debug: Dynamic var 
        scoreboard players operation y mcjava_sb_scope_0 = %1 mcjava_sb_scope_0

        #Debug: Dynamic var 
        scoreboard players operation x mcjava_sb_scope_0 = y mcjava_sb_scope_0

      but could be optimized to:
        #DEBUG: BinaryOp -> Arithmetic operation
        scoreboard players operation y mcjava_sb_scope_0 = x mcjava_sb_scope_0
        scoreboard players operation y mcjava_sb_scope_0 += 5 mcjava_sb_scope_0

        #Debug: Dynamic var 
        scoreboard players operation x mcjava_sb_scope_0 = y mcjava_sb_scope_0

      and this should apply to any variable declaration that is BinaryOperation -> if setting then our temporary variable
      is the variable that we are setting to
    */
    
    std::shared_ptr<VarInfo> generateBinaryOp(const BinaryOpNode& node) {
        //auto currentSb = getCurrentScoreboard();
        auto output = getCurrentOutput();
        
        // we can generate these 2 nodes because there is at least 1 variable -> analyzer combined all 2 constants binary operators
        VarInfo leftVar  = *node.left ->visit(*this);
        VarInfo rightVar = *node.right->visit(*this);

        // we dont want to change anything in variables -> just generate it
        //std::string tempVarName = getTempVarName();
        //node.varInfo->storagePath  = tempVarName;
        //node.varInfo->storageIdent = currentSb;

        std::string tempVarName = node.varInfo->storagePath;
        std::string tempVarSb   = node.varInfo->storageIdent;

        switch (node.op.type) 
        {
        case TokenType::PLUS : {
            // THERE IS NO WAY TO OPTIMIZE THIS FURTHER DUE TO MINECRAFT COMMAND LIMITATIONS
            // 2 commands is the minimum for addition and there isn't any noticable gain in performace

            if (rightVar.isConstant && leftVar.isConstant) {
                if (!options_.silent) std::cout << "GEN WARNING: Encountered both sides of addition being constant, they should have been folded by the analyzer\n";
                *output << "#Debug: Scoreboard ADD -> 2 constants\n";
                *output << "scoreboard players set " << tempVarName << " " << tempVarSb << " " << leftVar.constValue << "\n";
                *output << "scoreboard players add " << tempVarName << " " << tempVarSb << " " << rightVar.constValue << "\n";
                break;
            }

            if (rightVar.isConstant) {
                *output << "#Debug: Scoreboard ADD -> rightVar is constant\n";
                *output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent << "\n";
                *output << "scoreboard players add " << tempVarName << " " << tempVarSb << " " << rightVar.constValue << "\n"; // addition is alternating
                break;
            } else if (leftVar.isConstant) {
                *output << "#Debug: Scoreboard ADD -> leftVar is constant\n";
                *output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
                *output << "scoreboard players add " << tempVarName << " " << tempVarSb << " " << leftVar.constValue << "\n"; // addition is alternating
                break;
            }
            [[fallthrough]]; // fall to MULTIPLY / DIVIDE case
        }
        case TokenType::MINUS : {

            if (rightVar.isConstant && leftVar.isConstant) {
                if (!options_.silent) std::cout << "GEN WARNING: Encountered both sides of subtraction being constant, they should have been folded by the analyzer\n";
                *output << "#Debug: Scoreboard REMOVE -> 2 constants\n";
                *output << "scoreboard players set " << tempVarName << " " << tempVarSb << " " << leftVar.constValue << "\n";
                *output << "scoreboard players remove " << tempVarName << " " << tempVarSb << " " << rightVar.constValue << "\n";
                break;
            }

            // Optimalized: scoreboard players remove
            if (rightVar.isConstant) {
                *output << "#Debug: Scoreboard REMOVE -> rightVar is constant\n";
                *output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent << "\n";
                *output << "scoreboard players remove " << tempVarName << " " << tempVarSb << " " << rightVar.constValue << "\n";
                break;
            } else if (leftVar.isConstant) {
                *output << "#Debug: Scoreboard REMOVE -> leftVar is constant\n";
                // subtraction isn't alternating
                *output << "scoreboard players set " << tempVarName << " " << tempVarSb << " " << leftVar.constValue << "\n"; 
                *output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " -= " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
                break;
            }
            [[fallthrough]]; // fall to MULTIPLY / DIVIDE case
        }
        case TokenType::MULTIPLY :
        case TokenType::DIVIDE : {
            std::string comparator = node.op.value.value() + "=";

            // constants cannot help with performance in this case
            *output << "#DEBUG: BinaryOp -> Arithmetic operation\n";
            *output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent  << "\n";
            *output << "scoreboard players operation " << tempVarName << " " << tempVarSb
                    << " " << comparator << " "       << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            
            break;
        }

        case TokenType::LESS :
        case TokenType::GREATER :
        case TokenType::LESS_EQUAL :
        case TokenType::GREATER_EQUAL : {
            std::string comparator = node.op.value.value();

            if (leftVar.isConstant && rightVar.isConstant) {
                // handled by if(rightVar.isConstant) -> comparision needs at least one dynamic variable, so we cannot optimize it,
                // and it shouldn't appear because of constant folding 

                //error("Both sides of comparison operator cannot be constant in codegen, they should have been folded by the analyzer");
                //break;
            }
           
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
                *output << "execute store success score " << tempVarName << " " << tempVarSb 
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
                *output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << rightVar.storagePath << " " << rightVar.storageIdent
                    << " matches " << sValue << "\n";
                
                break;
            }

            *output << "#DEBUG: BinaryOp -> Default Comparison operation\n";
            *output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " " << comparator << " " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";

            break;
        } 
        
        case TokenType::EQUALS_EQUALS : {
            if (rightVar.isConstant) {
                std::string value = rightVar.constValue;

                *output << "#DEBUG: BinaryOp -> Equals Comparison operation -> RightVar is const\n";
                *output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            if (leftVar.isConstant) {
                std::string value = leftVar.constValue;

                *output << "#DEBUG: BinaryOp -> Equals Comparison operation -> LeftVar is const\n";
                *output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << rightVar.storagePath << " " << rightVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            *output << "#DEBUG: BinaryOp -> Default Equals Comparison operation\n";
            *output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            break;
        }
        case TokenType::NOT_EQUALS : {

            if (rightVar.isConstant) {
                std::string value = rightVar.constValue;

                *output << "#DEBUG: BinaryOp -> Not Equals Comparison operation -> RightVar is const\n";
                *output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute unless score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            if (leftVar.isConstant) {
                std::string value = leftVar.constValue;

                *output << "#DEBUG: BinaryOp -> Not Equals Comparison operation -> LeftVar is const\n";
                *output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute unless score " << rightVar.storagePath << " " << rightVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            *output << "#DEBUG: BinaryOp -> Default Not Equals Comparison operation\n";
            *output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute unless score " << leftVar.storagePath << " " << leftVar.storageIdent
                    << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            break;
        }
    
        default:
            error("Unknown Token Type in binary operator");
        }

        return node.varInfo;
    }


    void generateIfWithElse(const IfNode& node) {
        // schema:
        // if function 'then' returns 1 then run function 'else'
        // function then: return 1 unless condition is met

        // STATIC :
        if (node.isConditionConstant) {
            ASTNode* branch     = node.conditionValue == true ? node.thenBranch.get() : node.elseBranch.get();
            std::string comment = node.conditionValue == true ? "# Static Then Body\n" : "# Static Else Body\n";
            
            auto mainOutput = getCurrentOutput();
            *mainOutput << comment;
            appendBranch(branch);
    
            return;
        }

        /// DYNAMIC :
        VarInfo conditionVar = *node.condition->visit(*this);      

        // then branch
        std::string thenComment = "# Then Body\n";
        std::string thenAdditional = "execute unless score " + conditionVar.storagePath + " " + conditionVar.storageIdent + " matches 1 run return 1\n";
        std::string thenScopeName = generateBranch(node.thenBranch.get(), thenComment + thenAdditional);


        // else scope
        std::string elseComment = "# Else Body\n";
        std::string elseScopeName = generateBranch(node.elseBranch.get(), elseComment);

        auto mainOutput = getCurrentOutput();
        *mainOutput << "# Check condition  'if'\n";        
        // if thenScope branch returns 1 then execute else branch
        *mainOutput << "execute if function " << getFunctionNameSpace() << thenScopeName << " run function " << getFunctionNameSpace() << elseScopeName << "\n";
    }

    std::string generateBranch(ASTNode* body, const std::string& additionalBefore = "", const std::string& additionalAfter = "") {
        enterScope();
        std::string scopeName = getCurrentScope().name;
        auto output = getCurrentOutput();

        *output << additionalBefore;

        appendBranch(body);

        *output << additionalAfter;
        
        exitScope();
        return scopeName;
    }

    void appendBranch(ASTNode* body){
        auto scopeNode = dynamic_cast<ScopeNode*>(body);
        if (!scopeNode) {
            // single statement
            body->visit(*this);
        } else {
            // body is a scope
            for (const auto& stmt : scopeNode->statements) {
                stmt->visit(*this);
            }
        }
    }


    void generateOnlyIf(const IfNode& node) {
        // schema:
        // if condition then run function 'then'

        // check if the branch will even fire
        // NOTE: if we would want to implement debug mode or debbuger we need to let this pass so the loop body will be generated
        if (node.isConditionConstant && node.conditionValue == false) return;
        
        // then branch
        std::string comment = "# Then Body\n";
        std::string thenScopeName = generateBranch(node.thenBranch.get(), comment);
        

        auto mainOutput = getCurrentOutput();
        // first check to enter the loop
        *mainOutput << "# Check condition to enter the 'then' function\n";

        VarInfo conditionVar = *node.condition->visit(*this);        
        *mainOutput << "execute if score " << conditionVar.storagePath << " " << conditionVar.storageIdent << " matches 1 run function " << getFunctionNameSpace() << thenScopeName << "\n";
    
    }

    void generateWhile(const WhileNode& node) {
        // check if the loop will even start
        // NOTE: if we would want to implement debug mode or debbuger we need to let this pass so the loop body will be generated
        if (node.isConditionConstant && node.conditionValue == false) return;

        // loop scope
        enterScope();
        auto whileOutput = getCurrentOutput();
        std::string scopeName = getCurrentScope().name;

        // loop body
        *whileOutput << "# Loop Body\n";
        appendBranch(node.body.get());

        // recheck condition at the end of the loop
        *whileOutput << "# Recheck condition at the end of the loop\n";
        *whileOutput << prepareWhileCondition(node, scopeName);

        exitScope();
        
        
        // first check to enter the loop
        auto mainOutput = getCurrentOutput();
        *mainOutput << "# Check condition to enter the loop\n";
        *mainOutput << prepareWhileCondition(node, scopeName);
        
    }

    std::string prepareWhileCondition(const WhileNode& node, const std::string scopeName) {
        if (node.isConditionConstant) {
            // static condition check -> always the same
            if (node.conditionValue == true) {
                return "function " + getFunctionNameSpace() + scopeName + "\n";
            }
        } else {
            // generate condition and then check
            VarInfo conditionVar = *node.condition->visit(*this);        
            return "execute if score " + conditionVar.storagePath + " " + conditionVar.storageIdent + " matches 1 run function " + getFunctionNameSpace() + scopeName + "\n";
        }
        return "";
    }




    void generateScope(const ScopeNode& node) {
        enterScope();
                
        for (const auto& stmt : node.statements) {
            stmt->visit(*this);
        }
        
        exitScope();
    }

    
    // ===== SCOREBOARDS MANIPULATION =====

    std::string prepareScoreboards() {
        // Collect all unique scoreboard idents
        std::set<std::string> uniqueIdents;
        for (const auto& [name, var]: variables_) {
            uniqueIdents.insert(var->storageIdent);
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