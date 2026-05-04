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
        const Token& token = tokens[i];
        std::cout << token.line << ':' << token.col
                  << "  " << tokenTypeName(token.type)
                  << "  " << token.lexeme << "\n";
    }

    for (std::size_t i = 0; i < tokens.size(); i++) {
        if (isErrorToken(tokens[i])) {
            std::cerr << formatLexicalError(tokens[i]) << "\n";
            return 1;
        }
    }

    return 0;
}
