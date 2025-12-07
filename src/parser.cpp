#pragma once

#include <vector>
#include <variant>
#include <bits/stdc++.h>

#include "./tokenization.cpp"

#include "./CommandRegistry.hpp"

struct NodeExprIntLit {
    Token int_lit;
};
struct NodeExprIdent {
    Token ident;
};

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdent> var;
};

struct NodeCmdArg {
    // parser_name - np. "brigadier:integer" or "minecraft:entity" (opcjonalne)
    std::optional<std::string> parser;
    Token token; // surowy token z tokenizera (wartość + typ + line/col)
};

struct NodeStmtCommand {
    Token command_name;                 // token ident z nazwą komendy
    std::vector<NodeCmdArg> args;       // kolejność zgodna ze z matchowaną składnią
};

struct NodeStmtVar {
    Token ident;
    NodeExpr expr;
};

struct NodeStmt {
    std::variant<
        NodeStmtVar,
        NodeStmtCommand> var;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
};



class Parser {
public:
    inline explicit Parser(std::vector<Token> tokens, CommandRegistry& registry)
        : m_tokens(std::move(tokens)), reg(registry) {}

    std::optional<NodeExpr> parse_expr()
    {
        if (peek().has_value() && peek().value().type == TokenType::int_lit) {
            return NodeExpr{ .var = NodeExprIntLit { .int_lit = consume() } };
        } else if (peek().has_value() && peek().value().type == TokenType::ident) {
            return NodeExpr{ .var = NodeExprIdent { .ident = consume() } };
        } else {
            return {};
        }
    }

    // próbuj sparsować argument zgodnie z parser_name (proste heurystyki)
    std::optional<Token> try_parse_arg_for_parser(const std::optional<std::string>& parser_name) {
        if (!peek().has_value()) return {};
        Token tok = peek().value();
        if (tok.value.has_value()) {
            //std::cout << "ParsedTok - Token: " << tok.value.value() << std::endl;
        } else {
            //std::cout << "ParsedTok - Token: " << "Value not found! TokenType: " << tokenTypeToString(tok.type) << std::endl;
        }
        

        // heurystyka: rozpoznaj kilka parserów z data.json
        if (parser_name.has_value()) {
            const std::string &p = *parser_name;
            if (p.find("brigadier:integer") != std::string::npos ||
                p.find("brigadier:long") != std::string::npos) {
                if (tok.type == TokenType::int_lit) { return consume(); }
                return {};
            }
            if (p.find("brigadier:float") != std::string::npos ||
                p.find("brigadier:double") != std::string::npos) {
                // accept selector (@s, @p...) or ident (player name)
                if (tok.type == TokenType::float_lit || tok.type == TokenType::int_lit) return consume();
                return {};
            }
            if (p.find("minecraft:entity") != std::string::npos) {
                // accept selector (@s, @p...) or ident (player name)
                if (tok.type == TokenType::selector || tok.type == TokenType::ident) return consume();
                return {};
            }
            if (p.find("minecraft:block_pos") != std::string::npos) {
                std::optional<Token> t1 = peek();
                std::optional<Token> t2 = peek(1);
                std::optional<Token> t3 = peek(2);

                if (!t1.has_value() || !t2.has_value() || !t3.has_value()) return {};

                if ((t1->type == TokenType::coord || t1->type == TokenType::int_lit) &&
                    (t2->type == TokenType::coord || t2->type == TokenType::int_lit) &&
                    (t3->type == TokenType::coord || t3->type == TokenType::int_lit)) 
                {
                    // sprawdzenie spójności ^
                    bool allCaret = t1->value.value()[0] == '^' && t2->value.value()[0] == '^' && t3->value.value()[0] == '^';
                    bool anyCaret = t1->value.value()[0] == '^' || t2->value.value()[0] == '^' || t3->value.value()[0] == '^';

                    if (anyCaret && !allCaret) 
                        error(true, t1->line, t1->col, "Invalid block_pos: mix of ^ with other coordinates");
                    
                    std::stringstream str;
                    str << consume().value.value_or("") << " ";
                    str << consume().value.value_or("") << " ";
                    str << consume().value.value_or("");
                    return Token{ .type = TokenType::block_pos, .value = str.str(), .line = t1.value().line, .col = t1.value().col };
                }

                return {};
            }
            if (p.find("minecraft:vec3") != std::string::npos) {
                std::optional<Token> t1 = peek();
                std::optional<Token> t2 = peek(1);
                std::optional<Token> t3 = peek(2);

                if (!t1.has_value() || !t2.has_value() || !t3.has_value()) return {};

                if ((t1->type == TokenType::coord || t1->type == TokenType::int_lit || t1->type == TokenType::float_lit) &&
                    (t2->type == TokenType::coord || t2->type == TokenType::int_lit || t2->type == TokenType::float_lit) &&
                    (t3->type == TokenType::coord || t3->type == TokenType::int_lit || t3->type == TokenType::float_lit)) 
                {
                    // sprawdzenie spójności ^
                    bool allCaret = t1->value.value()[0] == '^' && t2->value.value()[0] == '^' && t3->value.value()[0] == '^';
                    bool anyCaret = t1->value.value()[0] == '^' || t2->value.value()[0] == '^' || t3->value.value()[0] == '^';

                    if (anyCaret && !allCaret) 
                        error(true, t1->line, t1->col, "Invalid vector3: mix of ^ with other coordinates");
                    
                    std::stringstream str;
                    str << consume().value.value_or("") << " ";
                    str << consume().value.value_or("") << " ";
                    str << consume().value.value_or("");
                    return Token{ .type = TokenType::block_pos, .value = str.str(), .line = t1.value().line, .col = t1.value().col };
                }

                return {};
            }
            if (p.find("minecraft:column_pos") != std::string::npos) {
                std::optional<Token> t1 = peek();
                std::optional<Token> t2 = peek(1);

                if (!t1.has_value() || !t2.has_value()) return {};

                if ((t1->type == TokenType::coord || t1->type == TokenType::int_lit || t1->type == TokenType::float_lit) &&
                    (t2->type == TokenType::coord || t2->type == TokenType::int_lit || t2->type == TokenType::float_lit)) 
                {
                    // sprawdzenie spójności ^
                    bool allCaret = t1->value.value()[0] == '^' && t2->value.value()[0] == '^';
                    bool anyCaret = t1->value.value()[0] == '^' || t2->value.value()[0] == '^';

                    if (anyCaret && !allCaret) 
                        error(true, t1->line, t1->col, "Invalid column_pos: mix of ^ with other coordinates");
                    
                    std::stringstream str;
                    str << consume().value.value_or("") << " ";
                    str << consume().value.value_or("");
                    return Token{ .type = TokenType::block_pos, .value = str.str(), .line = t1.value().line, .col = t1.value().col };
                }

                return {};
            }
            if (p.find("minecraft:rotation") != std::string::npos) {
                std::optional<Token> t1 = peek();
                std::optional<Token> t2 = peek(1);

                if (!t1.has_value() || !t2.has_value()) return {};

                if ((t1->type == TokenType::coord || t1->type == TokenType::int_lit || t1->type == TokenType::float_lit) &&
                    (t2->type == TokenType::coord || t2->type == TokenType::int_lit || t2->type == TokenType::float_lit)) 
                {
                    // sprawdzenie spójności ^
                    bool anyCaret = t1->value.value()[0] == '^' || t2->value.value()[0] == '^';

                    if (anyCaret) 
                        error(true, t1->line, t1->col, "Invalid rotation: '^' cannot be used in rotation");
                    
                    std::stringstream str;
                    str << consume().value.value_or("") << " ";
                    str << consume().value.value_or("");
                    return Token{ .type = TokenType::rotation, .value = str.str(), .line = t1.value().line, .col = t1.value().col };
                }

                return {};
            }
            if (p.find("minecraft:component") != std::string::npos) {
                // accept selector (@s, @p...) or ident (player name)
                if (tok.type == TokenType::ident || tok.type == TokenType::string_lit || tok.type == TokenType::component) return consume();
                return {};
            }
            if (p.find("minecraft:message") != std::string::npos) {
                if (tok.type == TokenType::string_lit || tok.type == TokenType::ident || tok.type == TokenType::selector) return consume();
                return {};
            }
            if (p.find("brigadier:string") != std::string::npos) {
                if (tok.type == TokenType::ident || tok.type == TokenType::string_lit) return consume();
                return {};
            }
            if (p.find("minecraft:item_predicate") != std::string::npos) {
                if (tok.type == TokenType::ident || tok.type == TokenType::asterisk || tok.type == TokenType::item_predicate) return consume();
                return {};
            }
            if (p.find("minecraft:block_predicate") != std::string::npos) {
                if (tok.type == TokenType::ident || tok.type == TokenType::asterisk || tok.type == TokenType::item_predicate) return consume();
                return {};
            }
            if (p.find("minecraft:resource_location") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::path) return consume();
                return {};
            }
            if (p.find("minecraft:block_state") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::block_state) return consume();
                return {};
            }
            if (p.find("minecraft:item_stack") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::block_state) return consume();
                return {};
            }
            if (p.find("minecraft:dimension") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::path) return consume();
                return {};
            }
            if (p.find("minecraft:resource_key") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::path) return consume();
                return {};
            }
            if (p.find("minecraft:resource_or_tag") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::item_predicate || tok.type == TokenType::path) return consume();
                return {};
            }
            if (p.find("minecraft:function") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::item_predicate || tok.type == TokenType::path) return consume();
                return {};
            }
            if (p.find("minecraft:loot_predicate") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::path) return consume();
                return {};
            }
            if (p.find("minecraft:objective") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident) return consume();
                return {};
            }
            if (p.find("minecraft:entity_type") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident) return consume();
                return {};
            }
            if (p.find("minecraft:uuid") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::uuid) return consume();
                return {};
            }
            if (p.find("minecraft:score_holder") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::selector || tok.type == TokenType::asterisk || tok.type == TokenType::item_predicate) return consume();
                return {};
            }
            if (p.find("minecraft:int_range") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::int_range || tok.type == TokenType::int_lit) return consume();
                return {};
            }
            if (p.find("minecraft:item_slots") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::item_predicate || tok.type == TokenType::path || tok.type == TokenType::item_slot) return consume();
                return {};
            }
            if (p.find("minecraft:resource") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::path) return consume();
                return {};
            }
            if (p.find("minecraft:game_profile") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident || tok.type == TokenType::selector) return consume();
                return {};
            }
            if (p.find("minecraft:nbt_path") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::component || tok.type == TokenType::json || tok.type == TokenType::string_lit || tok.type == TokenType::ident) return consume();
                return {};
            }
            if (p.find("minecraft:nbt_tag") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::component || tok.type == TokenType::json || tok.type == TokenType::string_lit || tok.type == TokenType::ident) return consume();
                return {};
            }
            if (p.find("minecraft:nbt_compound_tag") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::component || tok.type == TokenType::json || tok.type == TokenType::string_lit || tok.type == TokenType::ident) return consume();
                return {};
            }
            if (p.find("minecraft:dialog") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::component || tok.type == TokenType::json || tok.type == TokenType::ident) return consume();
                return {};
            }
            if (p.find("minecraft:gamemode") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident) {
                    if (!tok.value.has_value()) return {};
                    std::string t = tok.value.value();
                    if (t == "creative" || t == "adventure" || t == "survival" || t == "spectator")
                        return consume();
                }
                return {};
            }
            if (p.find("minecraft:entity_anchor") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident) {
                    if (!tok.value.has_value()) return {};
                    std::string t = tok.value.value();
                    if (t == "feet" || t == "eyes")
                        return consume();
                }
                return {};
            }
            if (p.find("brigadier:bool") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident) {
                    if (!tok.value.has_value()) return {};
                    std::string t = tok.value.value();
                    if (t == "true" || t == "false")
                        return consume();
                }
                return {};
            }
            if (p.find("minecraft:heightmap") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident) {
                    if (!tok.value.has_value()) return {};
                    std::string t = tok.value.value();
                    if (t == "motion_blocking_no_leaves" || t == "motion_blocking" || t == "ocean_floor" || t == "world_surface")
                        return consume();
                }
                return {};
            }
            if (p.find("minecraft:swizzle") != std::string::npos) {
                // resource locations often look like ident:ident or ident.ident -> accept ident
                if (tok.type == TokenType::ident) {
                    if (!tok.value.has_value()) return {};
                    std::string t = tok.value.value();
                    if (t == "x" || t == "xy" || t == "xyz" || t == "xz" || t == "xzy" || 
                        t == "y" || t == "yx" || t == "yxz" || t == "yz" || t == "yzx" || 
                        t == "z" || t == "zy" || t == "zyx" || t == "zx" || t == "zxy")
                        return consume();
                }
                return {};
            }
            // fallback: accept ident/int/selector/string for unknown parser
            if (tok.type == TokenType::ident || tok.type == TokenType::int_lit || tok.type == TokenType::selector || tok.type==TokenType::string_lit) {
                return consume();
            }
            return {};
        } else {
            // no parser specified -> accept ident/literal/int/selector/string
            if (tok.type == TokenType::ident || tok.type == TokenType::int_lit ||
                tok.type == TokenType::selector || tok.type==TokenType::string_lit) {
                return consume();
            }
            return {};
        }
    }

    std::optional<NodeStmt> parse_command() {
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
    }

    /*std::optional<NodeStmt> parse_command_old() {
        // zakładamy, że aktualny token to ident (nazwa komendy)
        if (!peek().has_value() || peek().value().type != TokenType::ident) return {};

        std::string cmdName = peek().value().value.value(); // nazwa komendy
        auto variants = reg.getSyntaxVariants(cmdName);
        if (variants.empty()) return {}; // nieznana komenda

        size_t start_idx = m_idx;

        // We'll try all variants and pick the best one (longest consumed, executable).
        bool anyMatched = false;
        size_t best_end_idx = start_idx;
        NodeStmtCommand best_cmdnode; // will be moved into return
        bool best_is_set = false;

        for (const auto &variant : variants) {
            m_idx = start_idx; // reset before trying this variant
            NodeStmtCommand cmdnode;
            // consume top-level command name token
            cmdnode.command_name = consume();

            bool fail = false;
            // we will collect args into local cmdnode.args as we go
            for (size_t i = 1; i < variant.size(); ++i) {
                const SyntaxToken &st = variant[i];

                std::cout << "Is " << cmdName << " literal - " << st.is_literal << "; Parser: " << st.parser.value_or(std::string{"UNKNOWN"}) << std::endl;
                if (st.is_literal) {
                    if (!peek().has_value()) { fail = true; break; }

                    Token t = peek().value();
                    //std::cout << "St.key -> " << st.key << std::endl;
                    if (!t.value.has_value() || t.value.value() != st.key) { fail = true; break; }

                    // consume and store literal as arg (so emitter prints it)
                    Token consumed = consume();
                    if (!consumed.value.has_value()) consumed.value = st.key;
                    NodeCmdArg a;
                    a.parser = std::nullopt;
                    a.token = std::move(consumed);
                    cmdnode.args.push_back(std::move(a));
                } else {
                    // argument node: try parse according to parser name
                    if (st.parser == "minecraft:message") {
                        // greedy parse: zbieramy wszystko aż do końca linii
                        std::stringstream joined;
                        bool first = true;

                        while (peek().has_value() && peek().value().type != TokenType::new_line && peek().value().type != TokenType::eof) {
                            std::cout << "Next Token" << std::endl;
                            Token t = consume();
                            if (t.value.has_value()) {
                                if (!first) joined << " ";
                                std::cout << "Joined: " << t.value.value() << std::endl;
                                joined << t.value.value();
                                first = false;
                            }
                        }

                        Token combined;
                        combined.type = TokenType::ident;  // albo ident, ale string_lit ładniej
                        combined.value = joined.str();

                        NodeCmdArg a;
                        a.parser = st.parser;
                        a.token = std::move(combined);
                        cmdnode.args.push_back(std::move(a));

                        // i tu KONIEC tej komendy — resztę linii ignorujemy jako część message
                        break;
                    } else {
                        auto parsedTok = try_parse_arg_for_parser(st.parser);
                        if (!parsedTok.has_value()) { fail = true; break; }
                        NodeCmdArg a;
                        a.parser = st.parser;
                        a.token = parsedTok.value();
                        cmdnode.args.push_back(std::move(a));
                    }
                }
            } // koniec pętli variant tokens

            // jeśli nie pasuje — kontynuuj kolejne warianty
            if (fail) continue;

            // successful match for this variant: record how far we consumed
            size_t end_idx = m_idx;
            bool executable_here = (!variant.empty() && variant.back().executable_here);

            // prefer variants that:
            // 1) match and are executable
            // 2) consume more tokens (longest match)
            if (executable_here) {
                if (!best_is_set || end_idx > best_end_idx) {
                    best_end_idx = end_idx;
                    best_cmdnode = std::move(cmdnode);
                    best_is_set = true;
                    anyMatched = true;
                }
            } else {
                // If not executable, we might still consider it if we have no better match yet,
                // but prefer executable ones over non-executable.
                if (!best_is_set && !anyMatched) {
                    best_end_idx = end_idx;
                    best_cmdnode = std::move(cmdnode);
                    best_is_set = true;
                    // mark matched but prefer to find executable later
                    anyMatched = true;
                }
            }
            // continue checking other variants to potentially find a longer/executable one
        } // koniec foreach variant

        if (!anyMatched) {
            // restore and return nothing
            m_idx = start_idx;
            return {};
        }

        // finalize: set parser index to end of best match
        m_idx = best_end_idx;

        // return NodeStmtCommand (wrapped in NodeStmt)
        return NodeStmt{ .var = best_cmdnode };
    }*/

    std::optional<NodeStmtVar> parse_stmt_var() {
        NodeStmtVar stmt_var = NodeStmtVar { .ident = consume() }; // consume ident
        consume(); // consume =
        if (auto expr = parse_expr()) {
            stmt_var.expr = expr.value();
        } else {
            error(peek().has_value(), peek().value().line, peek().value().col, "Invalid Expression");
        }
        return stmt_var;
    }

    std::optional<NodeStmt> parse_stmt()
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
    }

    std::optional<NodeProg> parse_prog() 
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
    }

private:
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

    // helpers: bezpieczny peek/consume pozostają
    inline bool tokensRemain() const { return peek().has_value(); }

    // zwraca tekst tokena (value) albo pusty string
    static std::string token_text(const Token& t) {
        return t.value.value_or(std::string{});
    }

    const std::vector<Token> m_tokens;
    size_t m_idx = 0;
    CommandRegistry& reg;

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
};