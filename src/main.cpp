#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include "./tokenization.hpp"
#include "./generation.cpp"
#include "./parser.cpp"

#include "./registries/SimplifiedCommandRegistry.hpp"

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
    if (args.find("dump-tokens") != args.end()) { // contains should work
        dumpTokens = true;
    }

    if (args.find("dump-cmds") != args.end()) { // contains should work
        dumpCmds = true;
    }

    // First time measurement
    clock_t tStart = clock();

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
    if (!reg.loadFromFile("./../mcdoc/commands.json", &err)) { std::cerr << "cmd load error: " << err << "\n"; return EXIT_FAILURE; }

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

    // end of parsing time measurement
    clock_t tEndPar = clock();

    // Generation
    Generator generator;
    {   
        std::fstream file(filename + ".mcfunction", std::ios::out);
        file << generator.generate(*ast);
    }

    // end of generation time measurement
    clock_t tEndGen = clock();

    // Print and format gathered times
    printf("Time parsing mcdoc: %.2fs\n", (double)(tEndReg - tStart)/CLOCKS_PER_SEC);
    printf("Time tokenizing: %.2fs\n", (double)(tEndTok - tEndReg)/CLOCKS_PER_SEC);
    printf("Time parsing: %.2fs\n", (double)(tEndPar - tEndTok)/CLOCKS_PER_SEC);
    printf("Time generating: %.2fs\n", (double)(tEndGen - tEndPar)/CLOCKS_PER_SEC);
    printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
    return EXIT_SUCCESS;
}