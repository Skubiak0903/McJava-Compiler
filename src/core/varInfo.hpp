// core/varInfo.hpp
#pragma once

#include <string>

enum class DataType {
    INT, FLOAT, BOOL, STRING, UNKNOWN
};

enum class VarStorageType {
    STORAGE, SCOREBOARD
};


struct VarInfo {
    std::string name;
    
    // --- Semantic Data ---
    DataType dataType;
    //int scopeLevel;      // Scope depth when the variable was initialized
    bool isConstant;
    std::string constValue;

    // --- Minecraft Data (Backend) ---
    VarStorageType storageType;
    std::string storageIdent;  
    std::string storagePath;
    
    // --- Additional Flags ---
    bool isUsed;
    bool isInitialized;
};


// =====  HELPER FUNCTIONS =====
inline std::string dataTypeToString(DataType type) {
    switch (type)
    {

    case DataType::INT  : return "Integer";
    case DataType::FLOAT    : return "Float";
    case DataType::BOOL     : return "Bool";
    case DataType::STRING   : return "String";
    case DataType::UNKNOWN  : return "Unknown";
    default                 : return "[UNKNOWN]";

    }
}