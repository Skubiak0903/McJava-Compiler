#include <unordered_map>

#include "./ast.hpp"

/*struct VisitReturn {
    DataType dataType;

    bool isConstant;
    std::string constValue;
};*/

class Analyzer : public ASTVisitor<std::shared_ptr<VarInfo>> {
private:
    std::unordered_map<std::string, std::shared_ptr<VarInfo>> variables_;

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

        auto varInfo = std::make_shared<VarInfo>();

        varInfo->name = varName;
        varInfo->dataType = resultVar->dataType;
        varInfo->isInitialized = true;
        varInfo->isConstant = resultVar->isConstant;

        if (resultVar->isConstant) {
            varInfo->constValue = resultVar->constValue;
        }
        
        // save variable
        //varInfo.scopeLevel = 
        
        variables_[varName] = varInfo;

        node.varInfo = varInfo;
        node.isAnalyzed = true;
    }

    std::shared_ptr<VarInfo> analyzeExpr(const ExprNode& node) {
        std::string tokValue = node.token.value.value();
        std::shared_ptr<VarInfo> varInfo;
        
        DataType dataType = DataType::UNKNOWN;
        bool isConstant = false;
        std::string constValue;
        
        switch (node.token.type)
        {
            case TokenType::IDENT : {
                // if ident then tokValue = varName
                // check if variable exists
                if (variables_.count(tokValue) <= 0) {
                    error("Tried to use unassigned variable " + tokValue);
                }
                varInfo = variables_[tokValue];
                varInfo->isUsed = true;
                
                dataType = varInfo->dataType;
                isConstant = varInfo->isConstant;
                if (varInfo->isConstant) constValue = varInfo->constValue;
                break;
            }
            case TokenType::INT_LIT :
                dataType = DataType::INT;
                isConstant = true;
                constValue = tokValue;
                break;
            
            case TokenType::FLOAT_LIT : 
                dataType = DataType::FLOAT;
                isConstant = true;
                constValue = tokValue;
                break;
            
            case TokenType::STRING_LIT :
                dataType = DataType::STRING;
                isConstant = true;
                constValue = tokValue;
                break;
            
            case TokenType::FALSE :
                dataType = DataType::BOOL;
                isConstant = true;
                constValue = "0";
                break;

            case TokenType::TRUE :
                dataType = DataType::BOOL;
                isConstant = true;
                constValue = "1";
                break;
            
            default:
            error("Got Expression node with unknown token type: " + tokenTypeToString(node.token.type));
        }
        
        if (!varInfo) {
            varInfo = std::make_shared<VarInfo>();
            
            varInfo->dataType = dataType;
            varInfo->isConstant = isConstant;
            varInfo->constValue = constValue;
            
        }
        
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
        bool isConstant = false;
        std::string constValue;

        if (dataType == DataType::UNKNOWN) {
            error("Not Matching types in binary operation: " + dataTypeToString(leftVar->dataType) + " and " + dataTypeToString(rightVar->dataType));
            return nullptr;
        }

        isConstant = leftVar->isConstant && rightVar->isConstant;
        if (isConstant) {
            //result->constValue = ;
            
            // just add normal switch (+,-,*,/) and compute constant expression
            
            // for now pretend its not constant
            isConstant = false;
            constValue = "";
        }

        auto varInfo = std::make_shared<VarInfo>();
        varInfo->name = "UNKNOWN-NAME[Internal error]";
        varInfo->dataType = dataType;
        varInfo->isUsed = false;
        varInfo->isConstant = isConstant;
        if (isConstant) varInfo->constValue = constValue;
        
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

private:
    [[noreturn]] void error(const std::string& msg) {
        std::cerr << "Analyzer error: " << msg << std::endl;
        exit(EXIT_FAILURE);
    }
};