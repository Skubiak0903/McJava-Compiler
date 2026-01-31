#include <memory>
#include <vector>
#include <unordered_map>

#include "./tokenization.hpp"
#include "./registries/SimplifiedCommandRegistry.hpp"
#include "./ast.hpp"

class Parser {
    public:
    explicit Parser(std::vector<Token> tokens, SimplifiedCommandRegistry& reg)
    : tokens_(std::move(tokens)), reg_(reg), pos_(0) {}
    
    std::unique_ptr<ASTNode> parse() {
        auto scope = std::make_unique<ScopeNode>();
        
        while (hasTokens()) {
            // Skip newlines and random semi colons
            if (peek().type == TokenType::NEW_LINE || peek().type == TokenType::SEMI_COLON){
                consume();
                continue;
            }
            
            // End if thats the end
            if (peek().type == TokenType::END_OF_FILE) break;
            
            // Parsuj statement i dodaj do listy
            if (auto stmt = parseStatement()) {
                scope->statements.push_back(std::move(stmt));
            } else {
                error(true, peek().line, peek().col, "Failed to parse statement: ", tokenTypeToString(peek().type));
            }
        }
        return scope;
        
    }

private:
    std::vector<Token> tokens_;
    SimplifiedCommandRegistry& reg_;
    size_t pos_;

    std::unordered_map<std::string, DataType> varTypes_;



    // ===== HELPER METHODS =====    
    

    void skipNewLines() {
        while(hasTokens() && (canSkip(peek().type) || peek().type == TokenType::END_OF_FILE)) consume();
    }

    bool canSkip(TokenType type) const {
        return type == TokenType::NEW_LINE || type == TokenType::SEMI_COLON;
    }

    bool isComparisonOperator(TokenType type) const {
        return type == TokenType::LESS ||
        type == TokenType::GREATER ||
        type == TokenType::LESS_EQUAL ||
        type == TokenType::GREATER_EQUAL ||
        type == TokenType::EQUALS_EQUALS ||
        type == TokenType::NOT_EQUALS;
    }

    

    // ===== PARSE LOGIC =====

    std::unique_ptr<ASTNode> parseStatement() {
        skipNewLines();

        if (!hasTokens()) return nullptr;

        Token tok = peek();

        // 1. Variable assignment (x = 5)
        if (tok.type == TokenType::IDENT && peek(1).type == TokenType::EQUALS) {
            return parseVarDecl();
        }
        
        // 2. Minecraft command (say "hello")
        if (tok.type == TokenType::CMD_KEY) {
            return parseCommand();
        }

        // 3. If statement
        if (tok.type == TokenType::IF) {
            return parseIf();
        }

        // 4. While loop
        if (tok.type == TokenType::WHILE) {
            return parseWhile();
        }
        
        
        // 5. Blok { ... }
        if (tok.type == TokenType::OPEN_BRACE) {
            return parseScope();
        }

        error(true, tok.line, tok.col, "Unknown statement type: ", tokenTypeToString(tok.type));
        return nullptr;
    }


    std::unique_ptr<ASTNode> parseVarDecl() {
        Token name = consume(); // consume IDENT
        consume(); // consume '='

        auto value = parseExpression();

        // save variable
        if (!name.value.has_value()) error("Encountered variable assignation with unknown name", name);
        std::string varName = name.value.value();
        varTypes_[varName] = value->dataType;

        return std::make_unique<VarDeclNode>(name, value->dataType, std::move(value));
    }


    std::unique_ptr<ASTNode> parseCommand() {
        Token cmdKey = consume(); // consume CMD_KEY
        auto node = std::make_unique<CommandNode>(cmdKey);

        // Zbierz wszystkie argumenty do końca linii
        while (hasTokens() && 
               peek().type != TokenType::NEW_LINE &&
               peek().type != TokenType::END_OF_FILE) {
            
            node->args.push_back(parseExpression());
        }

        skipNewLines();
        return node;
    }


    std::unique_ptr<ASTNode> parseIf() {
        consume(); // consume 'if'
        expect(TokenType::OPEN_PAREN, "after 'if'");
        consume(); // consume '('
        
        auto condition = parseExpression();

        expect(TokenType::CLOSE_PAREN, "after if condition");
        consume(); // consume ')'

        auto thenBranch = parseStatement();

        std::unique_ptr<ASTNode> elseBranch = nullptr;
        if (hasTokens() && peek().type == TokenType::ELSE) {
            consume(); // consume 'else'

            if (!hasTokens()) {
                error(true, peek().line, peek().col, "Expected 'if' or scope after 'else'");
            }

            if (peek().type == TokenType::OPEN_BRACE) {
                elseBranch = parseStatement();
            } else if (peek().type == TokenType::IF) {
                elseBranch = parseIf();
            } else {
                error(true, peek().line, peek().col, "Expected 'if' or scope after 'else', but got ", tokenTypeToString(peek().type));
            }
        }

        return std::make_unique<IfNode>(
            std::move(condition), 
            std::move(thenBranch), 
            std::move(elseBranch)
        );
    }


    std::unique_ptr<ASTNode> parseWhile() {
        consume(); // consume 'while'
        expect(TokenType::OPEN_PAREN, "after 'while'");
        consume(); // consume '('
        
        auto condition = parseExpression();

        expect(TokenType::CLOSE_PAREN, "after while condition");
        consume(); // consume ')'

        auto body = parseStatement();

        return std::make_unique<WhileNode>(
            std::move(condition), 
            std::move(body)
        );
    }

    std::unique_ptr<ASTNode> parseScope() {
        consume(); // consume '{'
        auto scope = std::make_unique<ScopeNode>();

        while (hasTokens() && peek().type != TokenType::CLOSE_BRACE) {
            if (auto stmt = parseStatement()) {
                scope->statements.push_back(std::move(stmt));
            }/* else {
                error("Failed to parse statement");
            }*/
           skipNewLines(); // needed -> witchout this there could be Tokens (NEW_LINE, CLOSE_BRACE) and becouse parseStatement skips new Lines it would fail becouse it doesnt know '}'
        }

        expect(TokenType::CLOSE_BRACE, "at end of the scope", peek(-1).line, peek(-1).col);
        consume(); // consume '}'

        return scope;
    }






    // ===== EXPRESSION PARSE LOGIC =====

    std::unique_ptr<ASTNode> parseExpression() {
        return parseComparison();  // Nowa funkcja która parsuje pełne wyrażenia
    }

    std::unique_ptr<ASTNode> parseComparison() {
        auto left = parseAdditive();
        
        while (hasTokens() && isComparisonOperator(peek().type)) {
            Token op = consume(); // consume operator
            auto right = parseAdditive();
            DataType type = inferBinaryOpType(op.type, left->dataType, right->dataType);

            left = std::make_unique<BinaryOpNode>(op, type, std::move(left), std::move(right));
        }
        
        return left;
    }

    std::unique_ptr<ASTNode> parseAdditive() {
        auto left = parseMultiplicative();
        
        while (hasTokens() && 
               (peek().type == TokenType::PLUS || 
                peek().type == TokenType::MINUS)) {
            
            Token op = consume();
            auto right = parseMultiplicative();
            DataType type = inferBinaryOpType(op.type, left->dataType, right->dataType);

            left = std::make_unique<BinaryOpNode>(op, type, std::move(left), std::move(right));
        }
        
        return left;
    }

    std::unique_ptr<ASTNode> parseMultiplicative() {
        auto left = parsePrimary();
        
        while (hasTokens() && 
               (peek().type == TokenType::MULTIPLY || 
                peek().type == TokenType::DIVIDE)) {
            
            Token op = consume();
            auto right = parsePrimary();
            DataType type = inferBinaryOpType(op.type, left->dataType, right->dataType);

            left = std::make_unique<BinaryOpNode>(op, type, std::move(left), std::move(right));
        }
        
        return left;
    }

    // Parsuje wyrażenie (liczba, string, zmienna)
    std::unique_ptr<ASTNode> parsePrimary() {
        if (!hasTokens()) error(false, 0, 0, "Expected expression");
        
        Token tok = consume();
        
        // Literały, identyfikatory, komendy
        if (tok.type == TokenType::INT_LIT ||
            tok.type == TokenType::FLOAT_LIT ||
            tok.type == TokenType::STRING_LIT ||
            // tok.type == TokenType::CMD_KEY ||
            tok.type == TokenType::TRUE ||
            tok.type == TokenType::FALSE ||
            tok.type == TokenType::IDENT) {

            if (tok.type == TokenType::IDENT) {
                // check if variable exists
                std::string varName = tok.value.value();
                if (varTypes_.find(varName) == varTypes_.end()) error("Tried to use unassigned variable (" + varName + ") in expression!", tok);

                // retrive variable
                DataType type = varTypes_[varName];
                return std::make_unique<ExprNode>(tok, type);
            }

            if (tok.type == TokenType::INT_LIT) return std::make_unique<ExprNode>(tok, DataType::INT);
            if (tok.type == TokenType::FLOAT_LIT) return std::make_unique<ExprNode>(tok, DataType::FLOAT);
            if (tok.type == TokenType::STRING_LIT) return std::make_unique<ExprNode>(tok, DataType::STRING);
            if (tok.type == TokenType::TRUE || tok.type == TokenType::FALSE) return std::make_unique<ExprNode>(tok, DataType::BOOL);

            error ("Unreachable");
            return std::make_unique<ExprNode>(tok, DataType::UNKNOWN);
        }

        // Nawiasy
        if (tok.type == TokenType::OPEN_PAREN) {
            auto expr = parseExpression();
            
            expect(TokenType::CLOSE_PAREN, "in expression");
            consume(); // consume ')'
            
            return expr;
        }

        // Unary minus (ex. -x )
        if (tok.type == TokenType::MINUS) {
            auto right = parsePrimary();

            if (right->dataType != DataType::INT && right->dataType != DataType::FLOAT) {
                error("Unary minus can only be applied to numbers", tok);
            }

            // zamień na to (0 - x)
            return std::make_unique<BinaryOpNode>(
                Token{TokenType::MINUS, "-", tok.line, tok.col}, 
                right->dataType,
                std::make_unique<ExprNode>(Token{TokenType::INT_LIT, "0", tok.line, tok.col}, DataType::INT),
                std::move(right)
            );
        }

        error(true, tok.line, tok.col, "Invalid expression");
        return nullptr;
    }

    // DataType helper
    DataType inferBinaryOpType(TokenType op, DataType leftType, DataType rightType) {
        // Dla operatorów arytmetycznych
        if (op == TokenType::PLUS || op == TokenType::MINUS ||
            op == TokenType::MULTIPLY || op == TokenType::DIVIDE) {
            
            // Reguły promocji typów
            if (leftType == DataType::FLOAT || rightType == DataType::FLOAT) {
                return DataType::FLOAT;  // float + int → float
            }
            if (leftType == DataType::INT && rightType == DataType::INT) {
                return DataType::INT;    // int + int → int
            }
            if (leftType == DataType::STRING && op == TokenType::PLUS) {
                return DataType::STRING; // string + string → string
            }
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

    
    // ===== TOKEN PEEK/CONSUME LOGIC =====
    bool hasTokens() const {
        return pos_ < tokens_.size();
    }

    Token peek(size_t offset = 0) const {
        if (pos_ + offset >= tokens_.size()) {
            return Token{TokenType::END_OF_FILE, "", 0, 0};
        }
        return tokens_[pos_ + offset];
    }

    Token consume() {
        if (!hasTokens()) error(false, 0, 0, "Unexpected end of file");
        // first get at pos_ then increment pos_ by 1
        return tokens_[pos_++];
    }

    // ===== REPORT METHODS =====

    template<typename... Args>
    [[noreturn]] void error(bool has_value, size_t line, size_t col, Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);  // fold expression w C++17
        if (has_value) {
            std::cerr << "Parser error: " << oss.str() << " at line " << line << ", column " << col << std::endl;
        } else {
            std::cerr << "Parser error: " << oss.str() << std::endl;
        }
        exit(EXIT_FAILURE);
    }

    [[noreturn]] void error(const std::string& msg, Token token = Token{}) {
        std::cerr << "Parser error: " << msg;
        if (token.type != TokenType::END_OF_FILE) {
            std::cerr << " at line " << token.line << ", col " << token.col;
        }
        std::cerr << std::endl;
        exit(EXIT_FAILURE);
    }

    void expect(TokenType expected, const std::string& context) {
        if (!hasTokens() || peek().type != expected) {
            error("Expected '" + std::string(tokenTypeToString(expected)) + "' " + context);
        }
    }

    void expect(TokenType expected, const std::string& context, size_t line, size_t col) {
        if (!hasTokens() || peek().type != expected) {
            error("Expected '" + std::string(tokenTypeToString(expected)) + "' " + context, Token{.line = line, .col = col});
        }
    }
};