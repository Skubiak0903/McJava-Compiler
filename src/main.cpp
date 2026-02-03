#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>

#include "./tokenization.hpp"
#include "./debug_generator.cpp"
#include "./generator.cpp"
#include "./parser.cpp"
#include "./analyzer.cpp"

#include "./registries/SimplifiedCommandRegistry.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{   
    if (argc < 2) {
        std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
        std::cerr << "mcjava.exe <input.mcjava> [args]" << std::endl;
        return EXIT_FAILURE;
    }

    // Get Arguments
    bool dumpTokens = false;
    bool dumpCmds = false;
    bool dumpParseTree = false;
    bool onlyAnalysis = false;
    bool silent = false;

    std::string mcdoc_path  = "./mcdoc/commands.json";
    std::string dp_prefix   = "mcjava";
    std::string dp_path     = "";

    std::unordered_map<std::string,std::string> args;
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.rfind("-", 0) == 0) {
            size_t eqPos = arg.find('=');
            if (eqPos != std::string::npos) {
                std::string key = arg.substr(1,eqPos-1);
                std::string value = arg.substr(eqPos+1);
                args[key] = value;
                //std::cout << "adding new " << key << " = " << value << " \n";
            } else {
                args[arg.substr(1)] = "true"; // np. -debug
            }
        }
    }

    // Match arguments
    if (args.find("dump-tokens") != args.end()) {
        dumpTokens = true;
    }

    if (args.find("dump-cmds") != args.end()) {
        dumpCmds = true;
    }

    if (args.find("dump-parse-tree") != args.end()) {
        dumpParseTree = true;
    }

    if (args.find("analysis") != args.end()) {
        onlyAnalysis = true;
    }

    if (args.find("silent") != args.end()) {
        silent = true;
    }



    if (args.find("mcdoc-path") != args.end()) {
        mcdoc_path = args["mcdoc-path"];
    }

    if (args.find("dp-prefix") != args.end()) {
        dp_prefix = args["dp-prefix"];
    }

    if (args.find("dp-path") != args.end()) {
        dp_path = args["dp-path"];
    }



    // First time measurement
    clock_t tStart = clock();
    auto realStart = std::chrono::steady_clock::now();

    std::string fullname = argv[1];
    std::string filename = fullname.substr(0, fullname.find_last_of("."));

    std::string contents;
    {
        std::ifstream input(argv[1], std::ios::in);
        std::string line;
        while (std::getline(input, line)) {
           contents += line;
           contents += '\n'; // zawsze LF
        }
    }

    // load simplified commands
    SimplifiedCommandRegistry reg;
    std::string err;
    if (!reg.loadFromFile(mcdoc_path, &err)) { std::cerr << "cmd load error: " << err << "\n"; return EXIT_FAILURE; }

    if (dumpCmds) {
        std::fstream file(filename + "-cmds.dump", std::ios::out);
        for (std::string cmd : reg.getRoots()) {
            file << cmd << std::endl;
        }
    }

    // End of mcdoc parsing time measurement
    clock_t tEndReg = clock();


    // Tokenization
    Tokenizer tokenizer(std::move(contents), reg);
    std::vector<Token> tokens = tokenizer.tokenize();

    // end of tokenization time measurement
    clock_t tEndTok = clock();

    // if dump tokens argument is set, dump all tokens to a  separate file
    if (dumpTokens) {
        std::fstream file(filename + "-token.dump", std::ios::out);
        for (Token token : tokens) {
            if (token.value.has_value()) {
                file << tokenTypeToString(token.type) << " -> " << token.value.value() << std::endl;
            } else {
                file << tokenTypeToString(token.type) << std::endl;
            }
        }
    }


    // Parsing tokens
    Parser parser(std::move(tokens), reg);
    auto ast = parser.parse(); // std::vector<ASTNode>
    
    if (!ast) {
        std::cerr << "Parse failed: no AST generated" << std::endl;
        return EXIT_FAILURE;
    }

    if (dumpParseTree) {
        std::ofstream file(filename + "-parser-tree.dump", std::ios::out);
        if (file.is_open()) {
            DebugGenerator debugGen(file);
            ast->visit(debugGen);
            file.close();
        }
    }

    // end of parsing time measurement
    clock_t tEndPar = clock();

    Analyzer analyzer;
    ast->visit(analyzer);
    const auto variables = analyzer.getVariables();

    clock_t tEndAnz = clock();

    // if -analysis then dont generate functions
    if (onlyAnalysis) {
        auto realEnd = std::chrono::steady_clock::now();
        
        // Print and format gathered times
        if (silent) return EXIT_SUCCESS;
        printf("Time parsing mcdoc: %.2fs\n", (double)(tEndReg - tStart)/CLOCKS_PER_SEC);
        printf("Time tokenizing: %.2fs\n", (double)(tEndTok - tEndReg)/CLOCKS_PER_SEC);
        printf("Time parsing: %.2fs\n", (double)(tEndPar - tEndTok)/CLOCKS_PER_SEC);
        printf("Time analyzing: %.2fs\n", (double)(tEndAnz - tEndPar)/CLOCKS_PER_SEC);
        printf("Time taken: %.4fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
        printf("Real time taken: %.4fs\n", std::chrono::duration<double>(realEnd - realStart).count());
        return EXIT_SUCCESS;
    }

    

    // Generation
    {   
        try {
            fs::create_directory(filename);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "FILE ERROR: " << e.what() << '\n';
        }

        fs::path path(filename);
        if (!silent) std::cout << "Path: " << path << "\n";
        FunctionGenerator funcGen(path, dp_prefix, dp_path, variables);
        ast->visit(funcGen);
    }
    
    // end of generation time measurement
    clock_t tEndGen = clock();
    auto realEnd = std::chrono::steady_clock::now();

     // Print and format gathered times
    if (silent) return EXIT_SUCCESS;
    printf("Time parsing mcdoc: %.2fs\n", (double)(tEndReg - tStart)/CLOCKS_PER_SEC);
    printf("Time tokenizing: %.2fs\n", (double)(tEndTok - tEndReg)/CLOCKS_PER_SEC);
    printf("Time parsing: %.2fs\n", (double)(tEndPar - tEndTok)/CLOCKS_PER_SEC);
    printf("Time analyzing: %.2fs\n", (double)(tEndAnz - tEndPar)/CLOCKS_PER_SEC);
    printf("Time generating: %.2fs\n", (double)(tEndGen - tEndPar)/CLOCKS_PER_SEC);
    printf("Time taken: %.4fs (CPU)\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
    printf("Real time taken: %.4fs\n", std::chrono::duration<double>(realEnd - realStart).count());
    return EXIT_SUCCESS;
}