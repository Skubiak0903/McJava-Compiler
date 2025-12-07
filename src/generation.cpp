#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "./parser.cpp"

// template for visitors
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;

class Generator {
public:
    explicit Generator(const NodeProg& prog)
        : m_prog(prog)
    {}

    // generowanie całego programu
    std::string gen_prog() {
        m_output.str("");
        m_output.clear();

        for (const NodeStmt& stmt : m_prog.stmts) {
            gen_stmt(stmt, m_output);
        }
        return m_output.str();
    }

private:
    const NodeProg& m_prog;
    std::stringstream m_output;

    // --- helpers ---
    static std::string gen_expr(const NodeExpr& expr) {
        return std::visit(overloaded {
            [](const NodeExprIntLit& e) {
                return e.int_lit.value.value();
            },
            [](const NodeExprIdent& e) {
                return e.ident.value.value();
            }
        }, expr.var);
    }

    static void gen_stmt(const NodeStmt& stmt, std::ostream& out) {
        std::visit(overloaded {
            [&](const NodeStmtVar& s) {
                emitVar(s, out);
            },            
            [&](const NodeStmtCommand& c) {
                emitCommand(c, out);
            }
        }, stmt.var);
    }

    static void emitCommand(const NodeStmtCommand& cmd, std::ostream& out) {
        // command_name token value
        std::string name = cmd.command_name.value.value_or(std::string{});
        out << name;
        for (const auto &arg : cmd.args) {
            out << " ";
            // emit depending on token type; if string_lit we may need quotes
            if (arg.token.type == TokenType::string_lit) {
                // add quotes around raw token value if tokenizer returned unquoted value
                out << '"' << arg.token.value.value() << '"';
            } else if (arg.token.type == TokenType::asterisk) {
                out << '*';
            } else {
                // for ident/int_lit/selector use raw token text
                if (!arg.token.value.has_value()) {
                    std::cout << "Napraw generator bo nie wszystko ma value! '" << tokenTypeToString(arg.token.type) << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                out << arg.token.value.value();
            }
        }
        out << "\n";
    }

    static void emitVar(const NodeStmtVar& stmt, std::ostream& out) {
        std::string name = stmt.ident.value.value();
        std::string val  = gen_expr(stmt.expr);
        out << "scoreboard players set " << name << " " << val << "\n";
    }
};