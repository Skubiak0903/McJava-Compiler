#pragma once

#include <vector>
#include <variant>
#include <bits/stdc++.h>

#include "./tokenization.hpp"
#include "./registries/SimplifiedCommandRegistry.hpp"
#include "./ast.hpp"

class Parser {
public:
    inline explicit Parser(std::vector<Token> tokens, SimplifiedCommandRegistry& registry)
        : m_tokens(std::move(tokens)), m_reg(registry) {}

    // Główna funkcja parsująca - zwraca AST
    std::unique_ptr<std::vector<ASTNode>> parse() {
        auto prog = std::make_unique<ASTNode>();
        // Tworzymy początkowy node programu
        std::vector<ASTNode> statements;

        while (peek().has_value()) {
            // Skip newlines and random semi colons
            if (peek()->type == TokenType::NEW_LINE || peek()->type == TokenType::SEMI_COLON){
                consume();
                continue;
            }
            
            // End if thats the end
            if (peek()->type == TokenType::END_OF_FILE) break;

            // Parsuj statement i dodaj do listy
            if (auto stmt = parseStatement()) {
                statements.push_back(std::move(*stmt));
            } else {
                error(true, peek()->line, peek()->col, "Failed to parse statement: ", tokenTypeToString(peek()->type));
            }
        }

         // Na razie zwracamy pierwszy statement lub puste
        if (!statements.empty()) {
            return std::make_unique<std::vector<ASTNode>>(std::move(statements));
        }
        return nullptr;
    }

    /*std::optional<NodeExpr> parse_expr()
    {
        if (peek().has_value() && peek().value().type == TokenType::int_lit) {
            return NodeExpr{ .var = NodeExprIntLit { .int_lit = consume() } };
        } else if (peek().has_value() && peek().value().type == TokenType::ident) {
            return NodeExpr{ .var = NodeExprIdent { .ident = consume() } };
        } else {
            return {};
        }
    }*/

    /*std::optional<NodeStmt> parse_command() {
        // zakładamy, że aktualny token to ident (nazwa komendy)
        if (!peek().has_value() || peek()->type != TokenType::ident) return {};

        size_t start_idx = m_idx;
        Token rootToken = consume(); // nazwa komendy
        const CmdNode* cur_node = reg.getRootNodeFor(rootToken.value.value());

        if (cur_node == nullptr) {
            m_idx = start_idx;
            return {}; // nieznana komenda
        }

        NodeStmtCommand stmt_node;
        stmt_node.command_name = rootToken;
        

        // jeśli węzeł sam w sobie jest wykonywalny i nie ma więcej tokenów,
        // to jest już poprawna komenda
        if (!peek().has_value() || peek()->type == TokenType::new_line || peek()->type == TokenType::eof) {
            if (cur_node->executable) return NodeStmt{ .var = stmt_node };
            // inaczej to nie jest kompletny wariant; przywróć i zwróć nullopt
            //m_idx = start_idx; - to jezeli był by return {};
            error(peek().has_value(), peek().value().line, peek().value().col, "Unexpected end of command!");
        }

        while (true) {
            // jeśli EOF/NEWLINE — sprawdź czy aktualny node jest executable
            auto opt = peek();
            if (!opt.has_value() || opt->type == TokenType::new_line || opt->type == TokenType::eof) {
                if (cur_node->executable) {
                    return NodeStmt{ .var = stmt_node };
                } else {
                    // niedokończona komenda — przywróć i zwróć nic
                    //m_idx = start_idx; - to jezeli był by return {};
                    error(peek().has_value(), peek().value().line, peek().value().col, "Unexpected end of command!");
                }
            }

            if (cur_node->key == "run") {
                const CmdNode* targetNode = reg.getRootNodeFor(opt->value.value_or(""));
                if (!targetNode) error(true, opt->line, opt->col, "Unexpected argument, got '", opt->value.value(), "'!");

                Token consumed = consume();
                NodeCmdArg a{ std::nullopt, consumed };
                stmt_node.args.push_back(std::move(a));
                cur_node = targetNode;
                continue;
            }

            bool redir_success = false;
            for (const std::string &redir : cur_node->redirect) {
                const CmdNode* targetNode = reg.getRootNodeFor(redir);
                if (!targetNode) continue; // should not happen

                const std::string &nextWord = opt->value.value_or("");
                auto litIt = targetNode->children.find(nextWord); 
                if (litIt == targetNode->children.end()) continue; // jeżeli nie znaleziono argumentu w  tym node

                redir_success = true;
                //std::cout << "znaleziono noda '" << redir << "' na lini " << opt->line << std::endl;

                Token consumed = consume();
                NodeCmdArg a{ std::nullopt, consumed };
                stmt_node.args.push_back(std::move(a));
                cur_node = litIt->second.get();
                break;
            }
            if (redir_success) continue;

            // jeżeli jest ident i arument to literal to poszukaj nextWord w pair<string,CmdNode> i zwróć jeżeli znalazł
            // najpierw spróbuj literalnego dopasowania O(1)
            if (opt->type == TokenType::ident) {
                const std::string &nextWord = opt->value.value_or("");
                auto litIt = cur_node->children.find(nextWord);
                if (litIt != cur_node->children.end() && litIt->second->type == "literal") {
                    // dopasowany literal
                    const CmdNode* next_node = litIt->second.get();
                    Token consumed = consume();
                    if (!consumed.value.has_value()) consumed.value = next_node->key;
                    NodeCmdArg a{ std::nullopt, std::move(consumed) };
                    stmt_node.args.push_back(std::move(a));
                    cur_node = next_node;
                    continue; // idź dalej od nowego cur_node
                }
            }

            // nie znaleziono literal — spróbuj argument children (w deterministycznej kolejności)
            bool anyArgumentChild = false;
            bool matchedArgument = false;


            // **IMPORTANT**: Iterate only argument-type children.
            // To zapewnić deterministyczną kolejność, możesz chcieć przechowywać
            // argument children w vector w CmdNode (zachowując order z JSON).
            for (const auto &p : cur_node->children) {
                const CmdNode* next_node = p.second.get();
                if (next_node->type != "argument") continue;
                anyArgumentChild = true;

                // specjalny-case: message => greedy consume to end-of-line and finish
                if (next_node->parser.has_value() && *next_node->parser == "minecraft:message") {
                    std::stringstream joined;
                    bool first = true;
                    while (peek().has_value() && peek()->type != TokenType::new_line && peek()->type != TokenType::eof) {
                        Token t = consume();
                        if (t.value.has_value()) {
                            if (!first) joined << " ";
                            joined << t.value.value();
                            first = false;
                        }
                    }
                    Token combined{ .type = TokenType::ident, .value = joined.str(), .line = rootToken.line, .col = rootToken.col };
                    NodeCmdArg a{ next_node->parser, std::move(combined) };
                    stmt_node.args.push_back(std::move(a));
                    // message uznajemy za terminal — update cur_node i zakończ (jak w wariantach)
                    cur_node = next_node;
                    // Po greedy parse traktujemy to jako koniec ścieżki; sprawdź wykonalność
                    if (cur_node->executable) return NodeStmt{ .var = stmt_node };
                    // jeśli po message nie ma executable, przywróć start i zwróć nullopt
                    //m_idx = start_idx; - to jezeli był by return {};
                    //return {};
                    error(true, opt->line, opt->col, "Found unexecutable command after greedy consume! Only way is to parse correctly is to remove that line!");
                }

                // zwykły argument: spróbuj go parsować heurystycznie
                auto parsedTok = try_parse_arg_for_parser(next_node->parser);
                if (!parsedTok.has_value()) {
                    // nie pasuje ten argument, spróbuj następnego argument child
                    continue;
                }

                // pasuje → zapisz i przejdź dalej (break z pętli children)
                NodeCmdArg a{ next_node->parser, parsedTok.value() };
                stmt_node.args.push_back(std::move(a));
                cur_node = next_node;
                matchedArgument = true;
                break;
            }

            if (matchedArgument) continue;

            // jeśli są argument children, ale żaden nie pasował => niezgodność
            if (anyArgumentChild) {
                //m_idx = start_idx;
                //return {}; // nie pasuje do żadnej ścieżki
                error(true, opt->line, opt->col, "Unexpected argument, got '", opt->value.value(), "'!"); 
            }

            // nie ma żadnej możliwej opcji (nie znaleziono literal ani argument)
            //m_idx = start_idx;
            //return {};
            error(peek().has_value(), opt->line, opt->col, "Found unknown argument '", opt->value.value(), "'!"); 
        } // koniec while
        return NodeStmt{ .var = stmt_node };
    }*/

    /*std::optional<NodeStmtVar> parse_stmt_var() {
        NodeStmtVar stmt_var = NodeStmtVar { .ident = consume() }; // consume ident
        consume(); // consume =
        if (auto expr = parse_expr()) {
            stmt_var.expr = expr.value();
        } else {
            error(peek().has_value(), peek().value().line, peek().value().col, "Invalid Expression");
        }
        return stmt_var;
    }*/

    /*std::optional<NodeStmt> parse_stmt()
    {   
        NodeStmt stmt;
        if (peek().has_value()) {
            if (peek().value().type == TokenType::ident) {
                // try parse as registered command
                if (auto cmd = parse_command()) stmt = cmd.value();
            }
            else if (peek().value().type == TokenType::ident && 
                peek(1).has_value() && peek(1).value().type == TokenType::equals) {
                    stmt.var = parse_stmt_var().value();
            } 
            else {
                error(true, peek().value().line, peek().value().col, "Unexpected Token '", tokenTypeToString(peek().value().type) ,"'");
            }

            if (peek().has_value() && peek().value().type == TokenType::new_line || peek().value().type == TokenType::eof) {
                consume();
            } else {
                error(peek().has_value(), peek().value().line, peek().value().col, "Expected 'New Line' after command but got '",peek().value().value.value(),"'");
            }
        }
        return stmt;
    }*/

    /*std::optional<NodeProg> parse_prog() 
    {
        NodeProg prog;
        while(peek().has_value()) {
            if (peek().value().type == TokenType::new_line || peek().value().type == TokenType::eof) {
                consume();
            }
            else if (auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            } else {
                error(peek().has_value(), peek().value().line, peek().value().col, "Invalid Statement");
            }
        }

        m_idx = 0;
        return prog;
    }*/

private:
    // parsing

    // Parse statement
    std::optional<ASTNode> parseStatement() {
        if (!peek().has_value()) return std::nullopt;

        Token tok = peek().value();

        // 1. Variable assignment (x = 5)
        if (tok.type == TokenType::IDENT && peek(1).has_value() && peek(1)->type == TokenType::EQUALS) { // its checking only for (IDENT =)
            return parseVariableAssignment();
        }
        
        // 2. Minecraft command (say "hello")
        if (tok.type == TokenType::CMD_KEY) {
            return parseMinecraftCommand();
        }

        error(true, tok.line, tok.col, "Unknown statement type: ", tokenTypeToString(tok.type));
        return std::nullopt;
    }


    // Parsuje przypisanie zmiennej: x = 5
    ASTNode parseVariableAssignment() {
        Token varName = consume();  // np. "myScore"
        consume();  // konsumuj '='
        
        // Parsuj wartość (może być liczba lub inna zmienna)
        ASTNode value = parseExpression();
        
        // TWORZYMY NODE AST!
        return make_vardecl(varName, std::move(value));
    }


    // Parse command (just first token is known rest is on user side to be sure its correct)
    ASTNode parseMinecraftCommand() {
        Token cmdToken = consume();  // np. "say"
        std::vector<ASTNode> args;
        
        // Zbierz wszystkie argumenty do końca linii
        while (peek().has_value() && 
               peek()->type != TokenType::NEW_LINE &&
               peek()->type != TokenType::END_OF_FILE) {
            
            args.push_back(parseExpression());
        }
        
        // Skip newline
        if (peek().has_value() && peek()->type == TokenType::NEW_LINE) {
            consume();
        }
        
        // TWORZYMY NODE AST!
        return make_command(cmdToken, std::move(args));
    }


    // Parsuje wyrażenie (liczba, string, zmienna)
    ASTNode parseExpression() {
        if (!peek().has_value()) error(false, 0, 0, "Expected expression");
        
        Token tok = consume();
        
        // Sprawdź typ tokena i utwórz odpowiedni node
        if (tok.type == TokenType::INT_LIT ||
            tok.type == TokenType::FLOAT_LIT ||
            tok.type == TokenType::STRING_LIT ||
            tok.type == TokenType::CMD_KEY ||       // without this `scoreboard players set player test 1` it would create error bc test is CMD_KEY as it is also separate command - the same thing would happen with /execute
            tok.type == TokenType::IDENT) {
            
            // TWORZYMY NODE AST!
            return make_expr(tok);
        }
        
        error(true, tok.line, tok.col, "Invalid expression token: ", tokenTypeToString(tok.type));
        return make_expr(Token{});
    }


    // Token consume/peek implementation
    inline std::optional<Token> peek(int offset = 0) const 
    {
        if (m_idx + offset >= m_tokens.size()) {
            return {};
        }
        return m_tokens.at(m_idx + offset);
    }

    inline Token consume() {
        // first get at m_idx then increment m_idx by 1
        return m_tokens.at(m_idx++);
    }


    // helpers

    // helpers: bezpieczny peek/consume pozostają
    //inline bool tokensRemain() const { return peek().has_value(); }

    // zwraca tekst tokena (value) albo pusty string
    /*static std::string token_text(const Token& t) {
        return t.value.value_or(std::string{});
    }*/


    template<typename... Args>
    [[noreturn]] void error(bool has_value, size_t line, size_t col, Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);  // fold expression w C++17
        if (has_value) {
            std::cerr << "Parser error: " << oss.str() << " at line " << line << ", column " << col << std::endl;
        } else {
            std::cerr << "Parser error: " << oss.str() << " at line " << "UNKNOWN" << ", column " << "UNKNOWN" << std::endl;
        }
        exit(EXIT_FAILURE);
    }

    // variables

    const std::vector<Token> m_tokens;
    size_t m_idx = 0;
    SimplifiedCommandRegistry& m_reg;
};