#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>

#include "./frontend/tokenizer.hpp"
#include "./frontend/parser.hpp"
#include "./middleend/analyzer.hpp"
#include "./backend/debug_generator.hpp"
#include "./backend/generator.hpp"

#include "./registries/SimplifiedCommandRegistry.hpp"
#include "./core/options.hpp"
#include "./core/token.hpp"
#include "./core/ast.hpp"

namespace fs = std::filesystem;

void printHelp() {
    std::cout << "Usage: mcjava <input.mcjava> [args]\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  -dump-tokens                Dump tokens to a file\n";
    std::cout << "  -dump-cmds                  Dump all commands list to a file\n";
    std::cout << "  -dump-parse-tree            Dump the parse tree to a file\n";
    std::cout << "  -dump-analyzer-tree         Dump the analyzer tree to a file\n";
    std::cout << "  -analysis                   Only perform analysis, skip generation\n";
    std::cout << "  -disable-constant-folding   Disable constant folding optimization\n";
    std::cout << "  -keep-unused-vars           Keep unused variables in output\n";
    std::cout << "  -silent                     Suppress all output except errors\n";
    std::cout << "  -mcdoc-path=<path>          Path to mcdoc commands.json (default: ./mcdoc/commands.json)\n";
    std::cout << "  -dp-prefix=<prefix>         Datapack function prefix (default: mcjava)\n";
    std::cout << "  -dp-path=<path>             Datapack function path (default: empty)\n";
}

int main(int argc, char* argv[])
{   
    // Check for help flag before other argument processing
    if (argc >= 2) {
        std::string firstArg = argv[1];
        if (firstArg == "-help" || firstArg == "--help") {
            printHelp();
            return EXIT_SUCCESS;
        }
    }

    if (argc < 2) {
        std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
        std::cerr << "mcjava <input.mcjava> [args]" << std::endl;
        std::cerr << "or use: mcjava -help" << std::endl;
        return EXIT_FAILURE;
    }

    Options options;

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
                args[arg.substr(1)] = "true"; // ex. -debug
            }
        }
    }

    auto hasFlag = [&args](const std::string& key) {
        return args.find(key) != args.end();
    };

    // Match arguments
    if (hasFlag("help")) {
        printHelp();
        return EXIT_SUCCESS;
    }

    // Dump
    if (hasFlag("dump-tokens"))         options.dumpTokens          = true;
    if (hasFlag("dump-cmds"))           options.dumpCmds            = true;
    if (hasFlag("dump-parse-tree"))     options.dumpParseTree       = true;
    if (hasFlag("dump-analyzer-tree"))  options.dumpAnalyzerTree    = true;

    // Analysis & Generation
    if (hasFlag("analysis"))                    options.onlyAnalysis        = true;
    if (hasFlag("disable-constant-folding"))    options.doConstantFolding   = false;
    if (hasFlag("keep-unused-vars"))            options.removeUnusedVars    = false;
    
    // Other
    if (hasFlag("silent")) options.silent = true;
    
    // Paths
    if (hasFlag("mcdoc-path")) options.mcdocPath = args["mcdoc-path"];
    if (hasFlag("dp-prefix")) options.dpPrefix = args["dp-prefix"];

    if (hasFlag("dp-path")) {
        options.dpPath = args["dp-path"];

        // make sure that the path always ends with '/'
        if (options.dpPath.length() > 0 && options.dpPath.back() != '/') {
            options.dpPath += '/';

            if (!options.silent) {
                printf("Info: Appended missing '/' to datapack path.\n");
            }
        }
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
    if (!reg.loadFromFile(options.mcdocPath, &err)) { std::cerr << "cmd load error: " << err << "\n"; return EXIT_FAILURE; }

    if (options.dumpCmds) {
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
    if (options.dumpTokens) {
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
    auto ast = parser.parse(); // std::unique_ptr<ASTNode>
    
    if (!ast) {
        std::cerr << "Parse failed: no AST generated" << std::endl;
        return EXIT_FAILURE;
    }

    if (options.dumpParseTree) {
        std::ofstream file(filename + "-parse-tree.dump", std::ios::out);
        if (file.is_open()) {
            DebugGenerator debugGen(file);
            debugGen.generate(*ast);
            file.close();
        }
    }

    // end of parsing time measurement
    clock_t tEndPar = clock();

    Analyzer analyzer(options);
    analyzer.analyze(*ast);
    const auto variables = analyzer.getVariables();

    if (options.dumpAnalyzerTree) {
        std::ofstream file(filename + "-analyzer-tree.dump", std::ios::out);
        if (file.is_open()) {
            DebugGenerator debugGen(file);
            debugGen.generate(*ast);
            file.close();
        }
    }

    clock_t tEndAnz = clock();

    // if -analysis then dont generate functions
    if (options.onlyAnalysis) {
        auto realEnd = std::chrono::steady_clock::now();
        
        // Print and format gathered times
        if (!options.silent) {
            printf("Time parsing mcdoc: %.2fs\n", (double)(tEndReg - tStart)/CLOCKS_PER_SEC);
            printf("Time tokenizing: %.2fs\n", (double)(tEndTok - tEndReg)/CLOCKS_PER_SEC);
            printf("Time parsing: %.2fs\n", (double)(tEndPar - tEndTok)/CLOCKS_PER_SEC);
            printf("Time analyzing: %.2fs\n", (double)(tEndAnz - tEndPar)/CLOCKS_PER_SEC);
            printf("Time taken: %.4fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
            printf("Real time taken: %.4fs\n", std::chrono::duration<double>(realEnd - realStart).count());
        }

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
        if (!options.silent) std::cout << "Path: " << path << "\n";
        FunctionGenerator funcGen(path, options, variables);
        funcGen.generate(*ast);
    }
    
    // end of generation time measurement
    clock_t tEndGen = clock();
    auto realEnd = std::chrono::steady_clock::now();

    // Print and format gathered times
    if (!options.silent) {
        printf("Time parsing mcdoc: %.2fs\n", (double)(tEndReg - tStart)/CLOCKS_PER_SEC);
        printf("Time tokenizing: %.2fs\n", (double)(tEndTok - tEndReg)/CLOCKS_PER_SEC);
        printf("Time parsing: %.2fs\n", (double)(tEndPar - tEndTok)/CLOCKS_PER_SEC);
        printf("Time analyzing: %.2fs\n", (double)(tEndAnz - tEndPar)/CLOCKS_PER_SEC);
        printf("Time generating: %.2fs\n", (double)(tEndGen - tEndAnz)/CLOCKS_PER_SEC);
        printf("Time taken: %.4fs (CPU)\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
        printf("Real time taken: %.4fs\n", std::chrono::duration<double>(realEnd - realStart).count());
    }

    return EXIT_SUCCESS;
}