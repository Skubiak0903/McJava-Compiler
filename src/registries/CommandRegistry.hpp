// CommandRegistry.hpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <iostream>
#include <optional>

#include "../../libs/json.hpp"
using json = nlohmann::json;

// One token in syntax: either a literal token (exact word) or an argument with parser
struct SyntaxToken {
    bool is_literal = true;
    std::string key;         // key in the JSON children map (e.g. "targets", "from", "*" or literal word)
    // if !is_literal:
    std::optional<std::string> parser;    // e.g. "minecraft:entity" or "brigadier:integer"
    std::optional<json> properties;       // raw properties object if present
    bool executable_here = false;         // marks if node with this token is executable
};

// A variant (one possible full syntax) is a sequence of SyntaxToken
using SyntaxVariant = std::vector<SyntaxToken>;

// Node of the command trie (one entry in children map)
struct CmdNode {
    std::string key; // key in parent->children map
    std::string type; // "literal" or "argument"
    bool executable = false;
    std::optional<std::string> parser; // for argument nodes
    std::optional<json> properties;    // optional properties object
    std::unordered_map<std::string, std::unique_ptr<CmdNode>> children;
    std::vector<std::string> redirect; // if present
};

class CommandRegistry {
public:
    CommandRegistry() = default;

    // Load data.json (the commands tree)
    bool loadFromFile(const std::string& path, std::string *err = nullptr) {
        std::ifstream f(path);
        if (!f.is_open()) {
            if (err) *err = "Cannot open file: " + path;
            return false;
        }
        try {
            json j; f >> j;
            // root may be object with "type":"root" and "children"
            if (j.is_object() && j.contains("children")) {
                auto &children = j["children"];
                for (auto it = children.begin(); it != children.end(); ++it) {
                    // top-level keys are command names: literal nodes
                    std::string cmdName = it.key();
                    auto node = parseNodeRecursive(it.key(), it.value());
                    roots.emplace(cmdName, std::move(node));
                }
                return true;
            } else {
                if (err) *err = "Unexpected JSON format: missing top-level children";
                return false;
            }
        } catch (const std::exception &ex) {
            if (err) *err = std::string("JSON parse error: ") + ex.what();
            return false;
        }
    }

    // Return list of top-level command names
    std::vector<std::string> rootCommands() const {
        std::vector<std::string> out;
        out.reserve(roots.size());
        for (auto &p : roots) out.push_back(p.first);
        return out;
    }

    const CmdNode* getRootNodeFor(const std::string& cmdName) const {
        auto it = roots.find(cmdName);
        if (it == roots.end()) return nullptr;
        
        return it->second.get();
    }

    // Get all syntax variants for command e.g. "teleport"
    // Each variant is a sequence of tokens (literal/argument) that ends at an executable node
    std::vector<SyntaxVariant> getSyntaxVariants(const std::string& cmdName) const {
        std::vector<SyntaxVariant> out;
        auto it = roots.find(cmdName);
        if (it == roots.end()) return out;
        SyntaxVariant cur;
        SyntaxToken topTok;
        topTok.is_literal = true;
        topTok.key = cmdName;
        topTok.executable_here = it->second->executable;
        cur.push_back(topTok);

        collectVariants(it->second.get(), cur, out, this); // <-- przekaż wskaźnik registry
        return out;
    }

    // Debug helper: print variants
    void printVariants(const std::string& cmdName, std::ostream& os = std::cout) const {
        auto vars = getSyntaxVariants(cmdName);
        if (vars.empty()) {
            os << "No variants for " << cmdName << "\n";
            return;
        }
        size_t idx = 0;
        for (auto &v : vars) {
            os << idx++ << ": ";
            for (size_t i=0;i<v.size();++i) {
                if (i) os << " ";
                if (v[i].is_literal) os << v[i].key;
                else {
                    os << "<" << v[i].key;
                    if (v[i].parser) os << ":" << *v[i].parser;
                    os << ">";
                }
            }
            os << (v.back().executable_here ? " [executable]" : "") << "\n";
        }
    }

    // Find a node by walking exact tokens (literals or accepting argument nodes)
    // This is a convenience to let your parser validate/consume tokens.
    // tokens: sequence of source tokens (strings).
    // Returns pair(found, executable_at_node)
    std::pair<bool,bool> matchTokens(const std::string& cmdName, const std::vector<std::string>& tokens) const {
        auto it = roots.find(cmdName);
        if (it == roots.end()) return {false,false};
        const CmdNode* node = it->second.get();
        size_t idx = 0;
        // first token corresponds to top-level command and is already matched
        if (idx < tokens.size() && tokens[0] == cmdName) ++idx;
        // walk remaining tokens
        while (idx < tokens.size()) {
            const std::string &tok = tokens[idx];
            // prefer exact literal child match
            auto litIt = node->children.find(tok);
            if (litIt != node->children.end()) {
                node = litIt->second.get();
                ++idx;
                continue;
            }
            // else try to match any argument child (type == "argument")
            bool consumed = false;
            for (auto &chp : node->children) {
                const CmdNode* child = chp.second.get();
                if (child->type == "argument") {
                    // we accept token as argument (we don't fully validate parser here)
                    node = child;
                    ++idx;
                    consumed = true;
                    break;
                }
            }
            if (!consumed) return {false,false};
        }
        return {true, node->executable};
    }

private:
    std::unordered_map<std::string, std::unique_ptr<CmdNode>> roots;

    // parse one JSON node into CmdNode recursively
    static std::unique_ptr<CmdNode> parseNodeRecursive(const std::string& key, const json& jnode) {
        auto n = std::make_unique<CmdNode>();
        n->key = key;
        if (jnode.contains("type") && jnode["type"].is_string()) n->type = jnode["type"].get<std::string>();
        if (jnode.contains("executable") && jnode["executable"].is_boolean()) n->executable = jnode["executable"].get<bool>();
        if (jnode.contains("parser") && jnode["parser"].is_string()) n->parser = jnode["parser"].get<std::string>();
        if (jnode.contains("properties")) n->properties = jnode["properties"];
        if (jnode.contains("redirect") && jnode["redirect"].is_array()) {
            for (auto &r : jnode["redirect"]) if (r.is_string()) n->redirect.push_back(r.get<std::string>());
        }
        if (jnode.contains("children") && jnode["children"].is_object()) {
            for (auto it = jnode["children"].begin(); it != jnode["children"].end(); ++it) {
                n->children.emplace(it.key(), parseNodeRecursive(it.key(), it.value()));
            }
        }
        return n;
    }

    // DFS collecting variants; cur already contains the sequence to the current node (including it)
    static void collectVariants(const CmdNode* node, SyntaxVariant& cur, std::vector<SyntaxVariant>& out, const CommandRegistry* reg = nullptr) {
    if (!cur.empty()) cur.back().executable_here = node->executable;
    if (node->executable) out.push_back(cur);

    // najpierw obsłuż dzieci normalne
    for (const auto &p : node->children) {
        const auto *child = p.second.get();
        SyntaxToken tok;
        tok.is_literal = (child->type == "literal");
        tok.key = child->key;
        tok.executable_here = child->executable;
        if (!tok.is_literal) {
            if (child->parser) tok.parser = child->parser;
            if (child->properties) tok.properties = *(child->properties);
        }
        cur.push_back(tok);

        // rekurencyjne wywołanie
        collectVariants(child, cur, out, reg);
        cur.pop_back();
    }

    // teraz obsłuż redirect
    if (!node->redirect.empty() && reg) {
        for (const auto &rname : node->redirect) {
            auto redirectedVariants = reg->getSyntaxVariants(rname);
            for (auto &rv : redirectedVariants) {
                SyntaxVariant combined = cur; // aktualny wariant
                // pomijamy pierwszy token (nazwę komendy redirect) aby nie dublować
                for (size_t i = 1; i < rv.size(); ++i) combined.push_back(rv[i]);
                out.push_back(combined);
            }
        }
    }
}
};
