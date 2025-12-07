#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include "./tokenization.cpp"
#include "./generation.cpp"
#include "./parser.cpp"

#include "./CommandRegistry.hpp"


inline void printToken(const Token& token);

int main(int argc, char* argv[])
{   
    if (argc < 2) {
        std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
        std::cerr << "mcjava <input.mcjava> [args]" << std::endl;
        return EXIT_FAILURE;
    }

    std::string arg = "";
    if (argc > 2) {
        arg = argv[2];
    }

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

    // load commands
    CommandRegistry reg;
    std::string err;
    if (!reg.loadFromFile("./mcdoc/commands.json", &err)) { std::cerr << "cmd load error: " << err << "\n"; return 1; }

    clock_t tEndDoc = clock();

    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();

    clock_t tEndTok = clock();   // koniec pomiaru

    if (arg == "-dump-tokens") {
        std::fstream file(filename + "-token.dump", std::ios::out);
        for (Token token : tokens) {
            if (token.value.has_value()) {
                file << tokenTypeToString(token.type) << " -> " << token.value.value() << std::endl;
            } else {
                file << tokenTypeToString(token.type) << std::endl;
            }
        }
    }

    Parser parser(std::move(tokens), reg);
    std::optional<NodeProg> root = parser.parse_prog();

    clock_t tEndPar = clock();   // koniec pomiaru

    if (!root.has_value()) {
        std::cerr << "No exit statement found!" << std::endl;
        return EXIT_FAILURE;
    }

    Generator generator(std::move(*root));
    {   
        std::fstream file(filename + ".mcfunction", std::ios::out);
        file << generator.gen_prog();
    }

    clock_t tEndGen = clock();   // koniec pomiaru

    printf("Time parsing mcdoc: %.2fs\n", (double)(tEndDoc - tStart)/CLOCKS_PER_SEC);
    printf("Time tokenizing: %.2fs\n", (double)(tEndTok - tEndDoc)/CLOCKS_PER_SEC);
    printf("Time parsing: %.2fs\n", (double)(tEndPar - tEndTok)/CLOCKS_PER_SEC);
    printf("Time generating: %.2fs\n", (double)(tEndGen - tEndPar)/CLOCKS_PER_SEC);
    printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
    return EXIT_SUCCESS;
}