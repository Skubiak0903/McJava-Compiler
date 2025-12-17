// SimplifiedCommandRegistry.hpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

#include "../../libs/json.hpp"
using json = nlohmann::json;

class SimplifiedCommandRegistry {
public:
    SimplifiedCommandRegistry() = default;

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
                    const json& jnode = it.value();
                    if (
                        !(jnode.contains("required_level") && jnode["required_level"].is_number_integer()) ||
                        jnode["required_level"].get<uint8_t>() > MAX_LEVEL
                    ) continue;
                    roots.push_back(cmdName);
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

    // Check if a command name is valid (exists in the registry)
    bool isValid(const std::string& cmdName) const {
        bool contains = roots.end() != std::find(roots.begin(), roots.end(), cmdName);
        return contains;
    }

    const std::vector<std::string> getRoots() const {
        return roots;
    }

private:
    static constexpr int MAX_LEVEL = 2;     // max allowed required_level for commands
    std::vector<std::string> roots;
};
