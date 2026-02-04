#include <unordered_map>

#include "./ast.hpp"

class Analyzer : public ASTVisitor<std::shared_ptr<VarInfo>> {
private:
    std::unordered_map<std::string, std::shared_ptr<VarInfo>> variables_;
    size_t tempVarCount = 0;


    std::string getCurrentScoreboard() const {
        // std::string scopeName = getCurrentScope().name;
        std::string scopeName = "scope_0";
        return "mcjava_sb_" + scopeName;
    }

    std::string getTempVarName() {
        return "%" + std::to_string(tempVarCount++);
    }

public:
    Analyzer() {}


    std::shared_ptr<VarInfo> visitCommandT(const CommandNode& node) override {
        analyzeCommand(node);
        return nullptr;
    }

    std::shared_ptr<VarInfo> visitVarDeclT(const VarDeclNode& node) override {
        analyzeVarDecl(node);
        return nullptr;
    }

    std::shared_ptr<VarInfo> visitExprT(const ExprNode& node) override {
        return analyzeExpr(node); 
    }

    std::shared_ptr<VarInfo> visitBinaryOpT(const BinaryOpNode& node) override {
        return analyzeBinaryOp(node);
    }

    std::shared_ptr<VarInfo> visitIfT(const IfNode& node) override {
        analyzeIf(node);
        return nullptr;
    }

    std::shared_ptr<VarInfo> visitWhileT(const WhileNode& node) override {
        analyzeWhile(node);
        return nullptr;
    }

    std::shared_ptr<VarInfo> visitScopeT(const ScopeNode& node) override {
        analyzeScope(node);
        return nullptr;
    }

    auto getVariables() {
        return variables_;
    }


private: 
    void analyzeCommand(const CommandNode& node) {
        for (const auto& arg : node.args) {
            arg->visit(*this); // Analyze all nodes
        }

        node.isAnalyzed = true;
    }

    void analyzeVarDecl(const VarDeclNode& node) {
        // analyze value first
        auto resultVar = node.value->visit(*this);
        std::string varName = node.name.value.value();
        
        if (varName.empty()) {
            error("VarDecl Error: Variable name is empty!");
        }

        if (!resultVar) error("VarDecl Error: Should be UNREACHABLE");

        if (resultVar->dataType == DataType::UNKNOWN) {
            error("VarDecl Error: Could not infer type of variable " + varName);
        }
        
        // redeclaration check -> we allow it
        //if (variables_.count(varName) > 0) {
        //}

        // set all data to be sure everything is correct
        VarInfo varData = { 
            .name           = varName,
            .dataType       = resultVar->dataType,
            
            .isConstant     = resultVar->isConstant,
            .constValue     = resultVar->constValue, // we can just set without checking if isConstant is true
            
            .storageType    = VarStorageType::SCOREBOARD, // for now we only support int so it will be fine with scoreboard
            .storageIdent   = getCurrentScoreboard(),
            .storagePath    = varName,
            
            .isUsed         = false, // if we redeclare the variable but it isnt used in expression then this will stay false
            .isInitialized  = true,
        };
        
        
        auto varInfo = std::make_shared<VarInfo>(varData);
        variables_[varName] = varInfo;

        node.varInfo = varInfo;
        node.isAnalyzed = true;
    }

    std::shared_ptr<VarInfo> analyzeExpr(const ExprNode& node) {
        std::string tokValue = node.token.value.value();

        // if ident then handle  it specially before creating VarInfo struct (that varData below)
        if (node.token.type == TokenType::IDENT) {
            // if ident then tokValue = varName

            // check if variable exists
            if (variables_.count(tokValue) <= 0) {
                error("Tried to use unassigned variable " + tokValue);
            }

            auto varInfo = variables_[tokValue]; // get original pointer
            varInfo->isUsed = true;

            // use force dynamic only for variable use
            //if (node.forceDynamic) varInfo-> isConstant = true; // FIXME: for some reason if its flipped it generates right
        
            node.varInfo = varInfo;
            node.isAnalyzed = true;
            return varInfo;
        } 

        // set all data to be sure everything is correct
        VarInfo varData = {
            .name          = "%const_" + tokValue, // its set but it should disappear in another steps of analyzing
            .dataType      = DataType::UNKNOWN,

            .isConstant    = false,
            .constValue    = "",
            
            .storageType   = VarStorageType::SCOREBOARD, // for now we only support int so it will be fine with scoreboard
            .storageIdent  = getCurrentScoreboard(),
            .storagePath   = "%const_" + tokValue, // its set but it should disappear in another steps of analyzing

            .isUsed        = false,
            .isInitialized = true,
        };
        
        switch (node.token.type)
        {
            case TokenType::INT_LIT :
                varData.dataType   = DataType::INT;
                varData.isConstant = true;
                varData.constValue = tokValue;
                break;
            
            case TokenType::FLOAT_LIT : 
                varData.dataType   = DataType::FLOAT;
                varData.isConstant = true;
                varData.constValue = tokValue;
                break;
            
            case TokenType::STRING_LIT :
                varData.dataType   = DataType::STRING;
                varData.isConstant = true;
                varData.constValue = tokValue;
                break;
            
            case TokenType::FALSE :
                varData.dataType   = DataType::BOOL;
                varData.isConstant = true;
                varData.constValue = "0";
                break;

            case TokenType::TRUE :
                varData.dataType   = DataType::BOOL;
                varData.isConstant = true;
                varData.constValue = "1";
                break;
            
            default:
            error("Got Expression node with unknown token type: " + tokenTypeToString(node.token.type));
        }

        auto varInfo = std::make_shared<VarInfo>(varData);
        
        node.varInfo = varInfo;
        node.isAnalyzed = true;
        return varInfo;
    }

    std::shared_ptr<VarInfo> analyzeBinaryOp(const BinaryOpNode& node){
        // Implementation of binary operation analysis
        auto leftVar = node.left->visit(*this);
        auto rightVar = node.right->visit(*this);

        if (!leftVar || !rightVar) {
            error("Failed to analyze binary operation");
            return nullptr;
        }

        DataType dataType = inferBinaryOpType(node.op.type, leftVar->dataType, rightVar->dataType);


        if (dataType == DataType::UNKNOWN) {
            error("Not Matching types in binary operation: " + dataTypeToString(leftVar->dataType) + " and " + dataTypeToString(rightVar->dataType));
            return nullptr;
        }

        bool isConstant = leftVar->isConstant && rightVar->isConstant;
        std::string constValue = "";

        std::string storagePath;  // if its constatn then it should disappear in another steps of analyzing
        if (isConstant) {
            //result->constValue = ;
            
            // just add normal switch (+,-,*,/) and compute constant expression
            
            // for now pretend its not constant
            isConstant = false;
            constValue = "";

            // disabled becouse constants folding is  not yet implemented and we treat constant binary operations as dynamic ones
            //storagePath = "%binaryOP"; // temp name that should not make it into generation
            storagePath = getTempVarName();
        } else {
            storagePath = getTempVarName();
        }


        // set all data to be sure everything is correct
        VarInfo varData = { 
            .name           = storagePath, 
            .dataType       = dataType,
            
            .isConstant     = isConstant,
            .constValue     = constValue,
            
            .storageType    = VarStorageType::SCOREBOARD, // for now we only support int so it will be fine with scoreboard
            .storageIdent   = getCurrentScoreboard(),
            .storagePath    = storagePath,
            
            .isUsed         = false,
            .isInitialized  = true,
        };

        auto varInfo = std::make_shared<VarInfo>(varData);
        
        node.varInfo = varInfo;
        node.isAnalyzed = true;
        return varInfo;
    }

    void analyzeIf(const IfNode& node) {
        node.condition->visit(*this);
        node.thenBranch->visit(*this);
        if (node.elseBranch) node.elseBranch->visit(*this);

        node.isAnalyzed = true;
    }

    void analyzeWhile(const WhileNode& node) {
        invalidateVarsInNode(node.body.get());
        
        // we need to first invalidate variables that were changed in the loop body
        // and then we can analyze condition and body with correct information about which variables are constant
        node.condition->visit(*this);
        node.body->visit(*this);
        
        node.isAnalyzed = true;
    }

    void analyzeScope(const ScopeNode& node) {
        for (const auto& arg : node.statements) {
            arg->visit(*this); // Analyze all nodes
        }
        node.isAnalyzed = true;
    }


    // DataType helper
    DataType inferBinaryOpType(TokenType op, DataType leftType, DataType rightType) {
        // arithmetic operators
        if (op == TokenType::PLUS || op == TokenType::MINUS ||
            op == TokenType::MULTIPLY || op == TokenType::DIVIDE) {
            
            /*
             *  EVERYTHING that is commented is not implemented at the moment
             */

            // float + int -> float
            //if (leftType == DataType::FLOAT && rightType == DataType::INT) return DataType::FLOAT;  
            //if (leftType == DataType::INT && rightType == DataType::FLOAT) return DataType::FLOAT;  
            
            // same types
            if (leftType == DataType::INT && rightType == DataType::INT) return DataType::INT;
            //if (leftType == DataType::FLOAT && rightType == DataType::FLOAT) return DataType::FLOAT;
            
            // string + string → string
            //if (leftType == DataType::STRING && op == TokenType::PLUS) return DataType::STRING;
            return DataType::UNKNOWN;
        }
        
        // Dla operatorów porównania
        if (op == TokenType::EQUALS_EQUALS || op == TokenType::NOT_EQUALS ||
            op == TokenType::LESS || op == TokenType::GREATER ||
            op == TokenType::LESS_EQUAL || op == TokenType::GREATER_EQUAL) {
            
            return DataType::BOOL;  // zawsze bool
        }
        
        return DataType::UNKNOWN;
    }

    void invalidateVarsInNode(ASTNode* node) {
        if (!node) return;

        // If declaration (int y = ...), mark as non-constant
        else if (auto decl = dynamic_cast<VarDeclNode*>(node)) {
            //std::cout << "Analyzer: Invalidate variable " << decl->name.value.value() << " as non-constant due to being in while loop body\n";
            //std::cout << "Analyzer: Variable " << decl->name.value.value() << " is now non-constant\n";
            invalidateVarsInNode(decl->value.get());
        }
        if (auto expr = dynamic_cast<ExprNode*>(node)) {
            // its double check is the TokenType is IDENT, another check is in analyzeExpr
            if (expr->token.type == TokenType::IDENT) {
                expr->forceDynamic = true;
            }
        }
        else if (auto bin = dynamic_cast<BinaryOpNode*>(node)) {
            invalidateVarsInNode(bin->left.get());
            invalidateVarsInNode(bin->right.get());
        }
        // If it is scope, do recursive invalidation for all statements
        else if (auto scope = dynamic_cast<ScopeNode*>(node)) {
            for (auto& stmt : scope->statements) invalidateVarsInNode(stmt.get());
        }
    }

private:
    [[noreturn]] void error(const std::string& msg) {
        std::cerr << "Analyzer error: " << msg << std::endl;
        exit(EXIT_FAILURE);
    }
};