// copre/scope.hpp
#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <memory>
#include <algorithm>

//temp
//#include <iostream>

#include "./varInfo.hpp"
#include "./funcInfo.hpp"

namespace fs = std::filesystem;

struct Scope {
    size_t id;
    std::string name;
    std::unordered_map<std::string, std::shared_ptr<VarInfo>> variables = {};
    std::unordered_map<std::string, std::shared_ptr<FuncInfo>> functions = {};
    std::shared_ptr<Scope> parent;

    // generator stuff
    fs::path path;
    std::stringstream output = std::stringstream();


    /*
        VARIABLES
    */

    // True if declared, false otherwise
    bool declareVar(const std::string& name, const std::shared_ptr<VarInfo>& varInfo) {
        if (lookupLocalVar(name) != nullptr) return false;
        
        // declare new variable in this scope 
        variables[name] = varInfo;
        return true;
    }


    // TRUE if updated, false otherwise
    bool assignVar(const std::string& name, const std::shared_ptr<VarInfo>& varInfo) {
        return updateVar(name, varInfo);
    }


    // returns if variable was updated successfully
    bool updateVar(const std::string& name, std::shared_ptr<VarInfo> newPtr) {
        // We are looking for map that contains this variable
        if (variables.count(name) > 0) {
            variables[name] = newPtr; // replace with new pointer, instead of copying whole object
            return true;
        } else if (parent) {
            return parent->updateVar(name, newPtr);
        }
        return false; // variable not found in any scope
    }

    // recursive lookup
    std::shared_ptr<VarInfo> lookupVar(const std::string& name) {
        auto local = lookupLocalVar(name);
        if (local != nullptr) return local;

        if (parent != nullptr) {
            return parent->lookupVar(name);
        }
        return nullptr;
    }

    // local lookup
    std::shared_ptr<VarInfo> lookupLocalVar(const std::string& name) {
        auto it = variables.find(name);
        if (it != variables.end()) {
            return it->second;
        }
        return nullptr;
    }

    /*
        FUNCTIONS
    */

    // True if declared, false otherwise
    bool declareFunc(const std::string& name, const std::shared_ptr<FuncInfo>& funcInfo) {
        if (lookupLocalFunc(name) != nullptr) return false;
        
        // declare new function in this scope 
        functions[name] = funcInfo;
        return true;
    }

    // recursive lookup
    std::shared_ptr<FuncInfo> lookupFunc(const std::string& name) {
        auto local = lookupLocalFunc(name);
        if (local != nullptr) return local;

        if (parent != nullptr) {
            return parent->lookupFunc(name);
        }
        return nullptr;
    }

    // local lookup, true found, false otherwise
    std::shared_ptr<FuncInfo> lookupLocalFunc(const std::string& name) {
        auto it = functions.find(name);
        if (it != functions.end()) {
            return it->second;
        }
        return nullptr;
    }
};