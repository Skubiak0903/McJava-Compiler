// frontend/parser.cpp
#include "./parser.hpp"

#include "./registries/SimplifiedCommandRegistry.hpp"
#include "./core/token.hpp"
#include "./core/ast.hpp"

class Parser::Impl {
public:
    Impl(std::vector<Token> tokens, SimplifiedCommandRegistry& reg)
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
            
            // Parse statement and add it to the global scope
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
    
    std::vector<Annotation> pendingAnnotations;


    // ===== HELPER METHODS =====   

    void skipNewLines() {
        while(hasTokens() && canSkip(peek().type)) consume();
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

        // 0. Annotations, @Annotation
        while (hasTokens() && peek().type == TokenType::ANNOTATION) {
            // dont return, just append all annotations into the list
            parseAnnotation();
            skipNewLines();
        }

        if (!hasTokens()) return nullptr;

        Token tok = peek();
        std::unique_ptr<ASTNode> node;

        // 1. Variable assignment (x = 5)
        if (tok.type == TokenType::IDENT && peek(1).type == TokenType::EQUALS) {
            node = parseVarDecl();
        }

        // 2. Minecraft command (say "hello")
        else if (tok.type == TokenType::CMD_KEY) {
            node = parseCommand();
        }

        // 3. If statement
        else if (tok.type == TokenType::IF) {
            node = parseIf();
        }

        // 4. While loop
        else if (tok.type == TokenType::WHILE) {
            node = parseWhile();
        }
        
        // 5. Scope { ... }
        else if (tok.type == TokenType::OPEN_BRACE) {
            node = parseScope();
        }
        

        // append annotations
        if (node) {
            if (!pendingAnnotations.empty()) {
                node->annotations = pendingAnnotations;
                pendingAnnotations.clear();
            }

            return node;
        }

        error(true, tok.line, tok.col, "Unknown statement type: ", tokenTypeToString(tok.type));
        return nullptr;
    }

    void parseAnnotation() {
        Token name = consume(); // consume ANNOTATION
        if (!name.value.has_value()) error("Encountered annotation without a name");

        // push_back annotation struct
        pendingAnnotations.push_back({name.value.value()});
    }


    std::unique_ptr<ASTNode> parseVarDecl() {
        Token name = consume(); // consume IDENT
        consume(); // consume '='

        if (!name.value.has_value()) error("Encountered variable assignation without name");
        auto value = parseExpression();

        return std::make_unique<VarDeclNode>(name, std::move(value));
    }


    std::unique_ptr<ASTNode> parseCommand() {
        Token cmdKey = consume(); // consume CMD_KEY
        auto node = std::make_unique<CommandNode>(cmdKey);

        // Collect all arguments to the end of the line or semi colon
        while (hasTokens() && 
               peek().type != TokenType::NEW_LINE &&
               peek().type != TokenType::END_OF_FILE &&
               peek().type != TokenType::SEMI_COLON) {
            
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
            }

           skipNewLines(); // needed -> without this there could be Tokens (NEW_LINE, CLOSE_BRACE) and because parseStatement skips newlines it would fail on '}'
        }

        expect(TokenType::CLOSE_BRACE, "at end of the scope", peek(-1).line, peek(-1).col);
        consume(); // consume '}'

        return scope;
    }






    // ===== EXPRESSION PARSE LOGIC =====

    std::unique_ptr<ASTNode> parseExpression() {
        return parseComparison();
    }

    std::unique_ptr<ASTNode> parseComparison() {
        auto left = parseAdditive();
        
        while (hasTokens() && isComparisonOperator(peek().type)) {
            Token op = consume();
            auto right = parseAdditive();

            left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right));
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

            left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right));
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

            left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right));
        }
        
        return left;
    }

    // Parses expressions (number, string, variable)
    std::unique_ptr<ASTNode> parsePrimary() {
        if (!hasTokens()) error(false, 0, 0, "Expected expression");
        
        Token tok = consume(); 

        switch (tok.type) {
            case TokenType::INT_LIT:
            case TokenType::FLOAT_LIT:
            case TokenType::STRING_LIT:
            case TokenType::TRUE:
            case TokenType::FALSE:
            case TokenType::IDENT:
                return std::make_unique<ExprNode>(tok);

            // brackets
            case TokenType::OPEN_PAREN: {
                auto expr = parseExpression();
                
                expect(TokenType::CLOSE_PAREN, "in expression");
                consume(); // consume ')'
                
                return expr;
            }

            // Unary minus (ex. -x )
            case TokenType::MINUS: {
                auto right = parsePrimary();

                // expand it (-x) -> (0 - x)
                return std::make_unique<BinaryOpNode>(
                    Token{TokenType::MINUS, "-", tok.line, tok.col}, 
                    std::make_unique<ExprNode>(Token{TokenType::INT_LIT, "0", tok.line, tok.col}),
                    std::move(right)
                );
            }

            default: {
                error(true, tok.line, tok.col, "Invalid expression");
                return nullptr;
            }
        }


        // should be unreachable -> left in case
        error(true, tok.line, tok.col, "INTERNAL ERROR: Reached unreachable code in parseExpression");        
        return nullptr;
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
        return tokens_[pos_++];
    }

    
    // ===== REPORT METHODS =====

    template<typename... Args>
    [[noreturn]] void error(bool has_value, size_t line, size_t col, Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
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

// ========== WRAPPER ==========
Parser::Parser(std::vector<Token> tokens, SimplifiedCommandRegistry& reg)
    : pImpl(std::make_unique<Impl>(tokens, reg)) {}

Parser::~Parser() = default;  // Needed for unique_ptr<Impl>

std::unique_ptr<ASTNode> Parser::parse() {
    return pImpl->parse();
}