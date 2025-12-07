// main.cpp
#include "CommandRegistry.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: cmdloader data.json\n";
        return 1;
    }
    std::string err;
    CommandRegistry reg;
    if (!reg.loadFromFile(argv[1], &err)) {
        std::cerr << "Failed to load: " << err << "\n";
        return 2;
    }

    auto roots = reg.rootCommands();
    std::cout << "Loaded " << roots.size() << " top-level commands (showing first 20):\n";
    //for (size_t i=0;i<std::min<size_t>(roots.size(),20);++i) std::cout << "  " << roots[i] << "\n";
    for (size_t i=0;i <roots.size();++i) std::cout << "  " << roots[i] << "\n";

    // Example: print syntax variants for teleport
    std::cout << "\nVariants for 'teleport':\n";
    reg.printVariants("teleport");

    // Example: match tokens
    std::vector<std::string> test1 = {"teleport", "@s", "0", "64", "0"};
    auto [found, exec] = reg.matchTokens("teleport", test1);
    std::cout << "matchTokens example -> found=" << found << " exec=" << exec << "\n";

    // Example: print syntax variants for teleport
    std::cout << "\nVariants for 'execute':\n";
    reg.printVariants("execute");

    // Example: match tokens
    std::vector<std::string> test2 = {"execute", "at", "@s", "run", "give", "@p", "minecraft:diamond_sword"};
    auto [found2, exec2] = reg.matchTokens("execute", test2);
    std::cout << "matchTokens example -> found=" << found2 << " exec=" << exec2 << "\n";

    return 0;
}
