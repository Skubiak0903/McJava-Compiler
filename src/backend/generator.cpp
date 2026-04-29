// backend/generator.cpp
#include "./generator.hpp"

#include <set>
#include <iostream>
#include <fstream>
#include <unordered_set>

#include "./../core/ast.hpp"
#include "./../core/options.hpp"
#include "./../core/visitor.hpp"
#include "./../core/scope.hpp"

class FunctionGenerator::Impl : public ASTVisitor {
private:
    const fs::path& path_;
    const Options& options_;

    const std::string functionNamespace_;

    std::unordered_set<std::shared_ptr<Scope>> allScopes_;
    
    std::shared_ptr<Scope> currentScope;
    std::stringstream& getOutput() {
        if (!currentScope) error("Tried to use unset currentScope!");
        std::cout << "Retrived output for '" << currentScope->name << "'\n";
        return currentScope->output;
    }

    std::shared_ptr<Scope> enterNewScope(const ASTNode& node) {
        return enterNewScope(node.scope);
    }

    std::shared_ptr<Scope> enterNewScope(const std::shared_ptr<Scope> scope) {
        auto newScope = std::make_shared<Scope>();

        newScope->id = nextScopeId++;
        newScope->name = "scope_" + std::to_string(newScope->id);
        newScope->parent = scope;
        newScope->isRoot = false;

        enterScope(newScope);
        return newScope;
    }
    
    void enterScope(const ASTNode& node) {
        enterScope(node.scope);
    }
    
    void enterScope(const std::shared_ptr<Scope> scope) {
        if (scope == nullptr) error("Tried to enter unexisting scope of a node");

        std::cout << "Entered scope '" << scope->name << "\n";
        
        std::string name = scope->isRoot ? "start.mcfunction" : (scope->name + ".mcfunction");
        scope->path = (path_ / name);
        
        allScopes_.insert(scope);
        currentScope = scope;
    }
    
    void exitScope(const ASTNode& node) {
        exitScope(node.scope);    
    }

    void exitScope(const std::shared_ptr<Scope> scope) {
        std::cout << "Exiting scope '" << scope->name << "' with path '" << scope->path << "'\n";
        
        currentScope = scope->parent; // for root it will be nullptr

        // nothing to generate
        if (scope->output.str().empty()) {
            if (!options_.silent) std::cout << "Scope '" << scope->name << "' is empty, skipping file generation.\n";
            return;
        }
        
        // save to file
        std::ofstream file(scope->path, std::ios::out);
        if (!file.is_open()) error("Could not save function file!");

        // last scope (global)
        if (scope->isRoot) {
            std::string body = scope->output.str();
            std::string header = prepareScoreboards();
            
            file << header << body;
        } else {
            file << scope->output.str();
        }

        file.close();
    }

    inline std::shared_ptr<VarInfo> visit(ASTNode& node) { return node.visit<std::shared_ptr<VarInfo>>(*this); }

public:

    Impl(fs::path& path, Options& options) 
        : path_(path), options_(options),  functionNamespace_(options_.dpPrefix + ":" + options_.dpPath) {}

    void generate(ASTNode& node) {
        visit(node);
    }

    ASTReturn visitCommand(const CommandNode& node) override {
        generateCommand(node);
        return nullptr;
    }

    ASTReturn visitVarDecl(const VarDeclNode& node) override {
        generateVarDecl(node);
        return nullptr;
    }

    ASTReturn visitVarAssign(const VarAssignNode& node) override {
        generateVarAssign(node);
        return nullptr;
    }

    ASTReturn visitExpr(const ExprNode& node) override {
        return generateExpr(node); 
    }

    ASTReturn visitBinaryOp(const BinaryOpNode& node) override {
        return generateBinaryOp(node);
    }

    ASTReturn visitIf(const IfNode& node) override {
        if (node.elseBranch) {
            generateIfWithElse(node);
        } else {
            generateOnlyIf(node);
        }
        return nullptr;
    }

    ASTReturn visitWhile(const WhileNode& node) override {
        generateWhile(node);
        return nullptr;
    }

    ASTReturn visitScope(const ScopeNode& node) override {
        generateScope(node);
        return nullptr;
    }

    ASTReturn visitFuncDecl(const FuncDeclNode& node) override {
        generateFuncDecl(node);
        return nullptr;
    }

    ASTReturn visitFuncCall(const FuncCallNode& node) override {
        generateFuncCall(node);
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

            auto tempVar = visit(*arg);
            ss << "{\"score\":{\"name\":\"" << tempVar->storagePath << "\",\"objective\":\"" << tempVar->storageIdent << "\"}},";
        }
        ss << "]";

        auto& output = getOutput();
        output << ss.str() << "\n";
    }
            
            
            
    void generateVarDecl(const VarDeclNode& node) {

        // dont emit unused variables
        if (!node.varInfo->isUsed && options_.removeUnusedVars && !node.varInfo->isExternal) {
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
        if (node.initValIsConst && node.varInfo->isUsed && options_.doConstantFolding && !node.varInfo->isExternal && options_.removeUnusedVars && node.varInfo->isConstant) { // we dont need to add node.varInfo->isUsed -> all unused were remove above
            return;
        }



        std::string varName = node.varInfo->name;
        auto& output = getOutput();

        // if is external check if value exists and if not then set it to default value
        if (node.varInfo->isExternal) {
            // execute store success score %i mcjava_sb_scope_0 run scoreboard players get started mcjava_sb_scope_0
            // execute if score %i mcjava_sb_scope_0 matches 0 run scoreboard players set %i mcjava_sb_scope_0 10

            output << "#Debug: External variable declaration " << varName << "\n";
            output << "execute store success score " << "%e" << " " << node.varInfo->storageIdent << " run scoreboard players get " << varName << " " << node.varInfo->storageIdent << "\n";
            
            if (!node.varInfo->constValue.empty()) { // needs to be like that, we don't know if its const by isConstant field, as it is  always false when value is external
                //output << "#Debug: Constant var [CONST: " + node.initValConstValue + "]\n";
                output << "execute if score %e " << node.varInfo->storageIdent << " matches 0 run scoreboard players set " << varName << " " << node.varInfo->storageIdent << " " << node.initValConstValue << "\n";
            } else {
                VarInfo tempVar = *visit(*node.value);
                
                output << "execute if score %e " << node.varInfo->storageIdent << " matches 0 run scoreboard players operation " << varName << " " << node.varInfo->storageIdent << " = " << tempVar.storagePath << " " << tempVar.storageIdent << "\n";
            }
            return;
        }

       

       
        if (node.initValIsConst) {
            output << "#Debug: Constant var [CONST: " + node.initValConstValue + "]\n";

            output << "scoreboard players set " << varName << " " << node.varInfo->storageIdent << " " << node.initValConstValue << "\n";
        } else {
            VarInfo tempVar = *visit(*node.value);

            output << "#Debug: Dynamic var\n";
            output << "scoreboard players operation " << varName << " " << node.varInfo->storageIdent << " = " << tempVar.storagePath << " " << tempVar.storageIdent << "\n";
        }
    }

    void generateVarAssign(const VarAssignNode& node) {
        // dont emit unused variables
        if (!node.varInfo->isUsed && options_.removeUnusedVars && !node.varInfo->isExternal) {
            return;
        }

        if (node.initValIsConst && node.varInfo->isUsed && options_.doConstantFolding && options_.removeUnusedVars && node.varInfo->isConstant && !node.varInfo->isExternal) { // we dont need to add node.varInfo->isUsed -> all unused were remove above
            return;
        }



        std::string varName = node.varInfo->name;
        auto& output = getOutput();
       
       
        if (node.initValIsConst) {
            output << "#Debug: Constant var assign [CONST: " + node.initValConstValue + "]\n";

            output << "scoreboard players set " << varName << " " << node.varInfo->storageIdent << " " << node.initValConstValue << "\n";
        } else {
            VarInfo tempVar = *visit(*node.value);

            output << "#Debug: Dynamic var assign\n";
            output << "scoreboard players operation " << varName << " " << node.varInfo->storageIdent << " = " << tempVar.storagePath << " " << tempVar.storageIdent << "\n";
        }
    }



    // LEFT UNREFACTORED FOR NOW -> NOT SURE IF THIS IS THE 100% CORRECT 
    std::shared_ptr<VarInfo> generateExpr(const ExprNode& node) {
        // just assigns value to variable
        std::string tokValue = node.token.value.value(); // variable name in user code
        //std::string varName = "%" +  tokValue; // won't collide with any user defined variables

        //auto& output = getOutput(node);

        // VarInfo varInfo = VarInfo{
        //     .storageType = VarStorageType::SCOREBOARD,
        //     .storageIdent = currentSb,
        //     .storagePath = varName,
        //     .isConstant = node.varInfo->isConstant,
        //     .constValue = node.varInfo->constValue,
        // };

        // if constant -> dont generate, higher node should implement it properly
        if(node.varInfo->isConstant || node.varInfo->isExternal) {
            
            // we dont want to change anything in variables -> just generate it
            //node.varInfo->storagePath = tokValue;
            //node.varInfo->storageIdent = getCurrentScoreboard();
            //output << "#Debug: Constant Expression [" + node.varInfo->constValue + "]\n";
            return node.varInfo;
        }

        // retrive variable -> variable exists because analyzer checked it
        //auto varInfo = variables_.at(node.token.value.value());
        auto varInfo = node.scope->lookupVar(tokValue);

        // can be wrong but we dont need to emits anything becouse we are only copying this value, and
        // the binary operation copy values that they change by themselves
        // and if we need to only copy the variable then we dont need to pass the real VarInfo highier 
        
        //output << "#Debug: Dynamic Expression\n";
        //output << "scoreboard players operation " << varName << " " << currentSb << " = " << varInfo.storagePath << " " << varInfo.storageIdent << "\n";

        //output << "#Debug: Node   Expression [" + node.varInfo->name + ", " + std::to_string(node.varInfo->isConstant) + "]\n";
        //output << "#Debug: Lookup Expression [" + varInfo->name + ", " + std::to_string(varInfo->isConstant) + "]\n";
        return varInfo;
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
        auto& output = getOutput();
        
        // we can generate these 2 nodes because there is at least 1 variable -> analyzer combined all 2 constants binary operators
        VarInfo leftVar  = *visit(*node.left);
        VarInfo rightVar = *visit(*node.right);

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
                output << "#Debug: Scoreboard ADD -> 2 constants\n";
                output << "scoreboard players set " << tempVarName << " " << tempVarSb << " " << leftVar.constValue << "\n";
                output << "scoreboard players add " << tempVarName << " " << tempVarSb << " " << rightVar.constValue << "\n";
                break;
            }

            if (rightVar.isConstant) {
                output << "#Debug: Scoreboard ADD -> rightVar is constant\n";
                output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent << "\n";
                output << "scoreboard players add " << tempVarName << " " << tempVarSb << " " << rightVar.constValue << "\n"; // addition is alternating
                break;
            } else if (leftVar.isConstant) {
                output << "#Debug: Scoreboard ADD -> leftVar is constant\n";
                output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
                output << "scoreboard players add " << tempVarName << " " << tempVarSb << " " << leftVar.constValue << "\n"; // addition is alternating
                break;
            }
            [[fallthrough]]; // fall to MULTIPLY / DIVIDE case
        }
        case TokenType::MINUS : {

            if (rightVar.isConstant && leftVar.isConstant) {
                if (!options_.silent) std::cout << "GEN WARNING: Encountered both sides of subtraction being constant, they should have been folded by the analyzer\n";
                output << "#Debug: Scoreboard REMOVE -> 2 constants\n";
                output << "scoreboard players set " << tempVarName << " " << tempVarSb << " " << leftVar.constValue << "\n";
                output << "scoreboard players remove " << tempVarName << " " << tempVarSb << " " << rightVar.constValue << "\n";
                break;
            }

            // Optimalized: scoreboard players remove
            if (rightVar.isConstant) {
                output << "#Debug: Scoreboard REMOVE -> rightVar is constant\n";
                output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent << "\n";
                output << "scoreboard players remove " << tempVarName << " " << tempVarSb << " " << rightVar.constValue << "\n";
                break;
            } else if (leftVar.isConstant) {
                output << "#Debug: Scoreboard REMOVE -> leftVar is constant\n";
                // subtraction isn't alternating
                output << "scoreboard players set " << tempVarName << " " << tempVarSb << " " << leftVar.constValue << "\n"; 
                output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " -= " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
                break;
            }
            [[fallthrough]]; // fall to MULTIPLY / DIVIDE case
        }
        case TokenType::MULTIPLY :
        case TokenType::DIVIDE : {
            std::string comparator = node.op.value.value() + "=";

            if (rightVar.isConstant) {
                output << "#Debug: BinaryOp -> Arithmetic operation (PREPARE) -> rightVar is constant\n";
                output << "scoreboard players set " << rightVar.storagePath << " " << rightVar.storageIdent << " " << rightVar.constValue << "\n";
            }

            // constants cannot help with performance in this case
            output << "#DEBUG: BinaryOp -> Arithmetic operation\n";
            output << "scoreboard players operation " << tempVarName << " " << tempVarSb << " = " << leftVar.storagePath << " " << leftVar.storageIdent  << "\n";
            output << "scoreboard players operation " << tempVarName << " " << tempVarSb
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

                output << "#DEBUG: BinaryOp -> Comparition operation -> RightVar is constant\n";
                output << "execute store success score " << tempVarName << " " << tempVarSb 
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

                output << "#DEBUG: BinaryOp -> Comparison operation -> LeftVar is constant\n";
                output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << rightVar.storagePath << " " << rightVar.storageIdent
                    << " matches " << sValue << "\n";
                
                break;
            }

            output << "#DEBUG: BinaryOp -> Default Comparison operation\n";
            output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " " << comparator << " " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";

            break;
        } 
        
        case TokenType::EQUALS_EQUALS : {
            if (rightVar.isConstant) {
                std::string value = rightVar.constValue;

                output << "#DEBUG: BinaryOp -> Equals Comparison operation -> RightVar is const\n";
                output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            if (leftVar.isConstant) {
                std::string value = leftVar.constValue;

                output << "#DEBUG: BinaryOp -> Equals Comparison operation -> LeftVar is const\n";
                output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << rightVar.storagePath << " " << rightVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            output << "#DEBUG: BinaryOp -> Default Equals Comparison operation\n";
            output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute if score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " = " << rightVar.storagePath << " " << rightVar.storageIdent << "\n";
            break;
        }
        case TokenType::NOT_EQUALS : {

            if (rightVar.isConstant) {
                std::string value = rightVar.constValue;

                output << "#DEBUG: BinaryOp -> Not Equals Comparison operation -> RightVar is const\n";
                output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute unless score " << leftVar.storagePath << " " << leftVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            if (leftVar.isConstant) {
                std::string value = leftVar.constValue;

                output << "#DEBUG: BinaryOp -> Not Equals Comparison operation -> LeftVar is const\n";
                output << "execute store success score " << tempVarName << " " << tempVarSb 
                    << " run execute unless score " << rightVar.storagePath << " " << rightVar.storageIdent 
                    << " matches " << value << "\n";

                break;
            }

            output << "#DEBUG: BinaryOp -> Default Not Equals Comparison operation\n";
            output << "execute store success score " << tempVarName << " " << tempVarSb 
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
            
            auto& mainOutput = getOutput();
            mainOutput << comment;
            appendBranch(branch);
    
            return;
        }

        /// DYNAMIC :
        VarInfo conditionVar = *visit(*node.condition);      


        // then branch
        std::string thenComment = "# Then Body\n";
        std::string thenAdditional = "execute unless score " + conditionVar.storagePath + " " + conditionVar.storageIdent + " matches 1 run return 1\n";
        auto thenScope = generateBranch(node.thenBranch.get(), thenComment + thenAdditional);

        
        // else scope
        std::string elseComment = "# Else Body\n";
        auto elseScope = generateBranch(node.elseBranch.get(), elseComment);


        auto& mainOutput = getOutput();
        mainOutput << "# Check condition  'if'\n";        
        // if thenScope branch returns 1 then execute else branch
        mainOutput << "execute if function " << functionNamespace_ << thenScope->name << " run function " << functionNamespace_ << elseScope->name << "\n";
    }

    std::shared_ptr<Scope> generateBranch(ASTNode* body, const std::string& additionalBefore = "", const std::string& additionalAfter = "") {
        // auto newScope = enterNewScope(*body);
        auto newScope = enterNewScope(currentScope);
        auto& output = getOutput();

        output << additionalBefore;

        appendBranch(body);

        output << additionalAfter;
        
        exitScope(newScope);
        return newScope;
    }

    void appendBranch(ASTNode* body){
        auto scopeNode = dynamic_cast<ScopeNode*>(body);
        if (!scopeNode) {
            // single statement
            visit(*body);
        } else {
            // body is a scope
            for (const auto& stmt : scopeNode->statements) {
                visit(*stmt);
            }
        }
    }


    void generateOnlyIf(const IfNode& node) {
        // schema:
        // if condition then run function 'then'

        // check if the branch will even fire
        // NOTE: if we would want to implement debug mode or debbuger we need to let this pass so the loop body will be generated
        
        // STATIC :
        if (node.isConditionConstant) {
            if (node.conditionValue == false) return;

            ASTNode* branch     = node.thenBranch.get();
            std::string comment = "# Static Then Body\n";
            
            auto& mainOutput = getOutput();
            mainOutput << comment;
            appendBranch(branch);
    
            return;
        } 
        

        // DYNAMIC :

        // then branch
        std::string comment = "# Then Body\n";
        auto thenScope = generateBranch(node.thenBranch.get(), comment);
        

        auto& mainOutput = getOutput();
        // first check to enter the loop
        mainOutput << "# Check condition to enter the 'then' function\n";

        VarInfo conditionVar = *visit(*node.condition);        
        mainOutput << "execute if score " << conditionVar.storagePath << " " << conditionVar.storageIdent << " matches 1 run function " << functionNamespace_ << thenScope->name << "\n";
    
    }

    void generateWhile(const WhileNode& node) {
        // check if the loop will even start
        // NOTE: if we would want to implement debug mode or debbuger we need to let this pass so the loop body will be generated
        if (node.isFirstCheckConstant && node.firstCheckConstValue == false) return;

        // loop scope
        auto newScope = enterNewScope(*node.body.get());
        auto& whileOutput = getOutput();
        std::string scopeName = newScope->name;

        // loop body
        whileOutput << "# Loop Body\n";
        appendBranch(node.body.get());

        // recheck condition at the end of the loop
        whileOutput << "# Recheck condition at the end of the loop\n";
        whileOutput << prepareWhileCondition(node, scopeName);

        exitScope(newScope);
        
        
        // first check to enter the loop
        auto& mainOutput = getOutput();
        mainOutput << "# Check condition to enter the loop\n";
        mainOutput << prepareWhileEnterCondition(node, scopeName);
        
    }

    std::string prepareWhileEnterCondition(const WhileNode& node, const std::string scopeName) {
        if (node.isFirstCheckConstant) {
            // static condition check -> always the same
            if (node.firstCheckConstValue == true) {
                return "function " + functionNamespace_ + scopeName + "\n";
            }
        } else {
            // generate condition and then check
            VarInfo conditionVar = *visit(*node.condition);        
            return "execute if score " + conditionVar.storagePath + " " + conditionVar.storageIdent + " matches 1 run function " + functionNamespace_ + scopeName + "\n";
        }
        return "";
    }

    std::string prepareWhileCondition(const WhileNode& node, const std::string scopeName) {
        if (node.isConditionConstant) {
            // static condition check -> always the same
            if (node.conditionValue == true) {
                return "function " + functionNamespace_ + scopeName + "\n";
            }
            return "";
        } else {
            // generate condition and then check
            VarInfo conditionVar = *visit(*node.condition);        
            return "execute if score " + conditionVar.storagePath + " " + conditionVar.storageIdent + " matches 1 run function " + functionNamespace_ + scopeName + "\n";
        }
    }




    void generateScope(const ScopeNode& node) {
        enterScope(node);
                
        for (const auto& stmt : node.statements) {
            std::cout << "Statement\n";
            visit(*stmt);
        }
        
        exitScope(node);
    }

    void generateFuncDecl(const FuncDeclNode& node) {
        std::string funcName = node.name.value.value();
        auto funcInfo = node.scope->lookupFunc(funcName);

        if (!funcInfo->isUsed) return;

        // function scope
        auto newScope = enterNewScope(currentScope);

        // function body
        auto& funcOutput = getOutput();
        funcOutput << "# Function body " << funcName << "\n";
        appendBranch(node.body.get());

        exitScope(newScope);
    }

    void generateFuncCall(const FuncCallNode& node) {
        std::string funcName = node.name.value.value();
        auto funcInfo = node.scope->lookupFunc(funcName);

        auto& funcOutput = getOutput();

        // function call
        funcOutput << "# call function " << funcInfo->name << "\n";
        funcOutput << "function " + functionNamespace_ + funcInfo->scopeName + "\n";
    }

    
    // ===== SCOREBOARDS MANIPULATION =====

    std::string prepareScoreboards() {
        // Collect all unique scoreboard idents
        std::set<std::string> uniqueIdents;
        for (const auto& scope : allScopes_) {
            for (const auto& [name, var]: scope->variables) {
                uniqueIdents.insert(var->storageIdent);
            }
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

// ========== WRAPPER ==========
FunctionGenerator::FunctionGenerator(fs::path& path, Options& options)
    : pImpl(std::make_unique<Impl>(path, options)) {}

FunctionGenerator::~FunctionGenerator() = default; // Needed for unique_ptr<Impl>

void FunctionGenerator::generate(ASTNode& node) {
    pImpl->generate(node);
}