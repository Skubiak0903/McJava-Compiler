#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>

struct Options {
    // Dumps
    bool dumpTokens         = false;
    bool dumpCmds           = false;
    bool dumpParseTree      = false;
    bool dumpAnalyzerTree   = false;

    // Analysis & Generation
    bool onlyAnalysis       = false;

    bool doConstantFolding  = true;
    bool removeUnusedVars   = true;
    
    //bool optimizeUniqueVars = true; // tries to reuse allocated vars as much as possible -> idk if this will gain any performace, its just an idea
    
    // Other
    bool silent = false;
    //bool debug = false;

    std::string mcdocPath  = "./mcdoc/commands.json";
    std::string dpPrefix   = "mcjava";
    std::string dpPath     = "";
};

#endif