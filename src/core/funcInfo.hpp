// core/varInfo.hpp
#pragma once

#include <string>


struct FuncInfo {
    std::string name;
    
    // functionName
    std::string scopeName;  
    
    // --- Additional Flags ---
    bool isUsed = false;
};