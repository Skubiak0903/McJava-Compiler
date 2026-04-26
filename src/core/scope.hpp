// copre/scope.hpp
#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <filesystem>
#include <memory>

//temp
#include <iostream>

#include "./varInfo.hpp"

namespace fs = std::filesystem;

struct Scope {
    size_t id;
    std::string name;
    std::unordered_map<std::string, std::shared_ptr<VarInfo>> variables = {};
    std::shared_ptr<Scope> parent;

    // generator stuff
    fs::path path;
    std::stringstream output = std::stringstream();


    // functions
    // True if declared, false otherwise
    bool declare(const std::string& name, const std::shared_ptr<VarInfo>& varInfo) {
        if (lookupLocal(name) != nullptr) return false;
        
        // declare new variable in this scope 
        variables[name] = varInfo;
        return true;
    }


    // TRUE if updated, false otherwise
    bool assign(const std::string& name, const std::shared_ptr<VarInfo>& varInfo) {
        return update(name, varInfo);
    }


    // returns if variable was updated successfully
    bool update(const std::string& name, std::shared_ptr<VarInfo> newPtr) {
        // We are looking for map that contains this variable
        if (variables.count(name) > 0) {
            // *existing = *varInfo;
            variables[name] = newPtr; // replace with new pointer, instead of copying whole object
            return true;
        } else if (parent) {
            return parent->update(name, newPtr);
        }
        return false; // variable not found in any scope
    }

    // recursive lookup
    std::shared_ptr<VarInfo> lookup(const std::string& name) {
        //std::cout << "Looking up variable '" << name << "' in scope '" << this->name << "'\n";
        // auto it = variables.find(name);
        // if (it != variables.end()) {
        //     //std::cout << "Found variable '" << name << "' in scope '" << this->name << "'\n";
        //     return it->second;
        // }

        auto local = lookupLocal(name);
        if (local != nullptr) return local;

        if (parent != nullptr) {
            //std::cout << "Variable '" << name << "' not found in scope '" << this->name << "'. Looking up in parent scope...\n";
            return parent->lookup(name);
        }
        //std::cout << "Variable '" << name << "' not found in any scope.\n";
        return nullptr;
    }

    // local lookup
    std::shared_ptr<VarInfo> lookupLocal(const std::string& name) {
        auto it = variables.find(name);
        if (it != variables.end()) {
            return it->second;
        }
        return nullptr;
    }
};