#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

enum TokenType {
    IF_KW,
    THEN_KW,
    ELSE_KW,
    END_KW,
    REPEAT_KW,
    UNTIL_KW,
    READ_KW,
    WRITE_KW,
    ID,
    NUM,
    STRING,
    ADDOP,
    SUBOP,
    MULOP,
    DIVOP,
    COMPARISONOP,
    ASSIGNMENTOP,
    SEMICOLON,
    COMMA,
    PUNCTUATION,
    EOF_TOKEN,
    ERROR_TOKEN
};

class Token {
public:
    TokenType type;
    std::string lexeme;
    int line;
    int col;

    Token() : type(ERROR_TOKEN), lexeme(""), line(0), col(0) {}

    Token(TokenType t, const std::string& l, int ln, int cl)
        : type(t), lexeme(l), line(ln), col(cl) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source)
        : source_(source), pos_(0), line_(1), col_(1) {}

    std::vector<Token> scanAll() {
        std::vector<Token> tokens;

        while (true) {
            Token t = nextToken();
            tokens.push_back(t);

            if (t.type == EOF_TOKEN) {
                break;
            }
        }

        return tokens;
    }

private:
    bool isAtEnd() const {
        return pos_ >= source_.size();
    }

    char peek() const {
        if (isAtEnd()) {
            return '\0';
        }
        return source_[pos_];
    }

    char peekNext() const {
        if (pos_ + 1 >= source_.size()) {
            return '\0';
        }
        return source_[pos_ + 1];
    }

    char advance() {
        if (isAtEnd()) {
            return '\0';
        }

        char c = source_[pos_];
        pos_++;

        if (c == '\n') {
            line_++;
            col_ = 1;
        } else {
            col_++;
        }

        return c;
    }
    // x=
    bool isLetter(char c) const {
        return std::isalpha(static_cast<unsigned char>(c)) != 0;
    }

    bool isDigit(char c) const {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    }

    Token makeToken(TokenType type, const std::string& lexeme, int line, int col) const {
        return Token(type, lexeme, line, col);
    }

    TokenType keywordType(const std::string& word) const {
        if (word == "if") return IF_KW;
        if (word == "then") return THEN_KW;
        if (word == "else") return ELSE_KW;
        if (word == "end") return END_KW;
        if (word == "repeat") return REPEAT_KW;
        if (word == "until") return UNTIL_KW;
        if (word == "read") return READ_KW;
        if (word == "write") return WRITE_KW;
        return ID;
    }
    // amr
    Token scanIdentifier() {
        int startLine = line_;
        int startCol = col_;
        std::string lexeme;

        while (!isAtEnd() && (isLetter(peek()) || isDigit(peek()))) {
            lexeme.push_back(advance());
        }

        return makeToken(keywordType(lexeme), lexeme, startLine, startCol);
    }

    Token scanNumber() {
        int startLine = line_;
        int startCol = col_;
        std::string lexeme;

        while (!isAtEnd() && isDigit(peek())) {
            lexeme.push_back(advance());
        }

        return makeToken(NUM, lexeme, startLine, startCol);
    }
    // "amr"
    Token scanString() {
        int startLine = line_;
        int startCol = col_;
        std::string lexeme;

        lexeme.push_back(advance());

        while (!isAtEnd() && peek() != '"') {
            lexeme.push_back(advance());
        }

        if (isAtEnd()) {
            return makeToken(ERROR_TOKEN, lexeme, startLine, startCol);
        }

        lexeme.push_back(advance());
        return makeToken(STRING, lexeme, startLine, startCol);
    }

    bool skipSpacesAndComments(Token& errorToken) {
        while (!isAtEnd()) {
            char c = peek();

            if (std::isspace(static_cast<unsigned char>(c))) {
                advance();
                continue;
            }

            if (c == '{') {
                int startLine = line_;
                int startCol = col_;
                advance();

                while (!isAtEnd() && peek() != '}') {
                    if (peek() == '{') {
                        // Consume nested opener and recover to the end of current comment.
                        advance();
                        while (!isAtEnd() && peek() != '}') {
                            advance();
                        }
                        if (!isAtEnd()) {
                            advance();
                        }
                        errorToken = makeToken(ERROR_TOKEN, "nested comment", startLine, startCol);
                        return false;
                    }
                    advance();
                }

                if (isAtEnd()) {
                    errorToken = makeToken(ERROR_TOKEN, "unclosed comment", startLine, startCol);
                    return false;
                }

                advance();
                continue;
            }

            break;
        }

        return true;
    }

    Token nextToken() {
        Token error;
        if (!skipSpacesAndComments(error)) {
            return error;
        }

        if (isAtEnd()) {
            return makeToken(EOF_TOKEN, "", line_, col_);
        }

        int startLine = line_;
        int startCol = col_;
        char c = peek();

        if (isLetter(c)) {
            return scanIdentifier();
        }

        if (isDigit(c)) {
            return scanNumber();
        }

        if (c == '"') {
            return scanString();
        }

        if (c == ':' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(ASSIGNMENTOP, ":=", startLine, startCol);
        }

        if (c == '<' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(COMPARISONOP, "<=", startLine, startCol);
        }

        if (c == '>' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(COMPARISONOP, ">=", startLine, startCol);
        }

        if (c == '!' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(COMPARISONOP, "!=", startLine, startCol);
        }

        advance();

        if (c == '+') return makeToken(ADDOP, "+", startLine, startCol);
        if (c == '-') return makeToken(SUBOP, "-", startLine, startCol);
        if (c == '*') return makeToken(MULOP, "*", startLine, startCol);
        if (c == '/') return makeToken(DIVOP, "/", startLine, startCol);
        if (c == '=' || c == '<' || c == '>') return makeToken(COMPARISONOP, std::string(1, c), startLine, startCol);
        if (c == ';') return makeToken(SEMICOLON, ";", startLine, startCol);
        if (c == ',') return makeToken(COMMA, ",", startLine, startCol);
        if (c == '(' || c == ')') return makeToken(PUNCTUATION, std::string(1, c), startLine, startCol);

        return makeToken(ERROR_TOKEN, std::string(1, c), startLine, startCol);
    }

    std::string source_;
    std::size_t pos_;
    int line_;
    int col_;
};

std::string tokenTypeName(TokenType type) {
    if (type == IF_KW) return "IF_KW";
    if (type == THEN_KW) return "THEN_KW";
    if (type == ELSE_KW) return "ELSE_KW";
    if (type == END_KW) return "END_KW";
    if (type == REPEAT_KW) return "REPEAT_KW";
    if (type == UNTIL_KW) return "UNTIL_KW";
    if (type == READ_KW) return "READ_KW";
    if (type == WRITE_KW) return "WRITE_KW";
    if (type == ID) return "ID";
    if (type == NUM) return "NUM";
    if (type == STRING) return "STRING";
    if (type == ADDOP) return "ADDOP";
    if (type == SUBOP) return "SUBOP";
    if (type == MULOP) return "MULOP";
    if (type == DIVOP) return "DIVOP";
    if (type == COMPARISONOP) return "COMPARISONOP";
    if (type == ASSIGNMENTOP) return "ASSIGNMENTOP";
    if (type == SEMICOLON) return "SEMICOLON";
    if (type == COMMA) return "COMMA";
    if (type == PUNCTUATION) return "PUNCTUATION";
    if (type == EOF_TOKEN) return "EOF";
    return "ERROR";
}

std::string readFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) {
        return "";
    }

    std::string content;
    char ch;
    while (file.get(ch)) {
        content.push_back(ch);
    }
    return content;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input-file>\n";
        return 1;
    }

    std::string source = readFile(argv[1]);
    if (source.empty()) {
        std::cerr << "Error: cannot read file or file is empty.\n";
        return 1;
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanAll();

    for (std::size_t i = 0; i < tokens.size(); i++) {
        const Token& t = tokens[i];
        std::cout << t.line << ':' << t.col << "  " << tokenTypeName(t.type) << "  " << t.lexeme << "\n";
    }

    return 0;
}
