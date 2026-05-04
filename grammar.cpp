#include "tiny_core.h"

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input-file>\n";
        return 1;
    }

    std::string source = readTextFile(argv[1]);
    if (source.empty()) {
        std::cerr << "Error: cannot read file or file is empty.\n";
        return 1;
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanAll();

    for (std::size_t i = 0; i < tokens.size(); i++) {
        if (isErrorToken(tokens[i])) {
            std::cerr << formatLexicalError(tokens[i]) << "\n";
            std::cout << "Lexical error(s) found\n";
            return 1;
        }
    }

    Parser parser(tokens);
    if (!parser.parse()) {
        std::cerr << formatSyntaxError(parser) << "\n";
        std::cout << "Syntax error(s) found\n";
        return 1;
    }

    std::cout << "OK - Valid Tiny program\n";
    return 0;
}
