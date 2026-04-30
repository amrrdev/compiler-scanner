#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

enum TokenType {
    IF_KW, THEN_KW, ELSE_KW, END_KW,
    REPEAT_KW, UNTIL_KW, READ_KW, WRITE_KW,
    ID, NUM, STRING,
    ADDOP, SUBOP, MULOP, DIVOP,
    COMPARISONOP, ASSIGNMENTOP,
    SEMICOLON, COMMA, PUNCTUATION,
    EOF_TOKEN, ERROR_TOKEN
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::string message;
    int line;
    int col;
};

class Lexer {
public:
    explicit Lexer(const std::string& source)
        : source(source), pos(0), line(1), col(1) {}

    std::vector<Token> scanAll() {
        std::vector<Token> tokens;

        while (true) {
            Token token = nextToken();
            tokens.push_back(token);

            if (token.type == EOF_TOKEN || token.type == ERROR_TOKEN) {
                break;
            }
        }

        return tokens;
    }

private:
    std::string source;
    std::size_t pos;
    int line;
    int col;

    bool isAtEnd() const {
        return pos >= source.size();
    }

    char peek() const {
        if (isAtEnd()) {
            return '\0';
        }
        return source[pos];
    }

    char peekNext() const {
        if (pos + 1 >= source.size()) {
            return '\0';
        }
        return source[pos + 1];
    }

    char advance() {
        if (isAtEnd()) {
            return '\0';
        }

        char ch = source[pos];
        pos++;

        if (ch == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }

        return ch;
    }

    bool isLetter(char ch) const {
        return std::isalpha(static_cast<unsigned char>(ch)) != 0;
    }

    bool isDigit(char ch) const {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    }

    Token makeToken(TokenType type, const std::string& lexeme, int tokenLine, int tokenCol,
                    const std::string& message = "") const {
        Token token;
        token.type = type;
        token.lexeme = lexeme;
        token.message = message;
        token.line = tokenLine;
        token.col = tokenCol;
        return token;
    }

    TokenType wordType(const std::string& word) const {
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

    Token scanIdentifier() {
        int tokenLine = line;
        int tokenCol = col;
        std::string lexeme;

        while (!isAtEnd() && (isLetter(peek()) || isDigit(peek()))) {
            lexeme.push_back(advance());
        }

        return makeToken(wordType(lexeme), lexeme, tokenLine, tokenCol);
    }

    Token scanNumber() {
        int tokenLine = line;
        int tokenCol = col;
        std::string lexeme;

        while (!isAtEnd() && isDigit(peek())) {
            lexeme.push_back(advance());
        }

        return makeToken(NUM, lexeme, tokenLine, tokenCol);
    }

    Token scanString() {
        int tokenLine = line;
        int tokenCol = col;
        std::string lexeme;

        lexeme.push_back(advance());

        while (!isAtEnd() && peek() != '"') {
            lexeme.push_back(advance());
        }

        if (isAtEnd()) {
            return makeToken(ERROR_TOKEN, lexeme, tokenLine, tokenCol, "Unclosed string");
        }

        lexeme.push_back(advance());
        return makeToken(STRING, lexeme, tokenLine, tokenCol);
    }

    bool skipIgnoredText(Token& errorToken) {
        while (!isAtEnd()) {
            char ch = peek();

            if (std::isspace(static_cast<unsigned char>(ch))) {
                advance();
                continue;
            }

            if (ch == '{') {
                int tokenLine = line;
                int tokenCol = col;
                advance();

                while (!isAtEnd() && peek() != '}') {
                    if (peek() == '{') {
                        errorToken = makeToken(ERROR_TOKEN, "{", tokenLine, tokenCol,
                                               "Nested comments are not allowed");
                        return false;
                    }
                    advance();
                }

                if (isAtEnd()) {
                    errorToken = makeToken(ERROR_TOKEN, "{", tokenLine, tokenCol,
                                           "Unclosed comment");
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
        Token errorToken;
        if (!skipIgnoredText(errorToken)) {
            return errorToken;
        }

        if (isAtEnd()) {
            return makeToken(EOF_TOKEN, "", line, col);
        }

        int tokenLine = line;
        int tokenCol = col;
        char ch = peek();

        if (isLetter(ch)) {
            return scanIdentifier();
        }

        if (isDigit(ch)) {
            return scanNumber();
        }

        if (ch == '"') {
            return scanString();
        }

        if (ch == ':' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(ASSIGNMENTOP, ":=", tokenLine, tokenCol);
        }

        if (ch == '<' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(COMPARISONOP, "<=", tokenLine, tokenCol);
        }

        if (ch == '>' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(COMPARISONOP, ">=", tokenLine, tokenCol);
        }

        if (ch == '!' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(COMPARISONOP, "!=", tokenLine, tokenCol);
        }

        advance();

        if (ch == '+') return makeToken(ADDOP, "+", tokenLine, tokenCol);
        if (ch == '-') return makeToken(SUBOP, "-", tokenLine, tokenCol);
        if (ch == '*') return makeToken(MULOP, "*", tokenLine, tokenCol);
        if (ch == '/') return makeToken(DIVOP, "/", tokenLine, tokenCol);
        if (ch == '=' || ch == '<' || ch == '>') return makeToken(COMPARISONOP, std::string(1, ch), tokenLine, tokenCol);
        if (ch == ';') return makeToken(SEMICOLON, ";", tokenLine, tokenCol);
        if (ch == ',') return makeToken(COMMA, ",", tokenLine, tokenCol);
        if (ch == '(' || ch == ')') return makeToken(PUNCTUATION, std::string(1, ch), tokenLine, tokenCol);

        return makeToken(ERROR_TOKEN, std::string(1, ch), tokenLine, tokenCol, "Invalid character");
    }
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens)
        : tokens(tokens), pos(0), hasError(false) {}

    std::string parse() {
        program();

        if (!hasError && !isAtEnd()) {
            reportError("Unexpected token after statement");
        }

        if (hasError) {
            return "Syntax error(s) found";
        }

        return "OK - Valid Tiny program";
    }

private:
    const std::vector<Token>& tokens;
    std::size_t pos;
    bool hasError;

    Token current() const {
        if (pos < tokens.size()) {
            return tokens[pos];
        }
        return {EOF_TOKEN, "", "", 0, 0};
    }

    bool isAtEnd() const {
        return current().type == EOF_TOKEN;
    }

    bool check(TokenType type) const {
        return current().type == type;
    }

    bool match(TokenType type) {
        if (!check(type)) {
            return false;
        }

        pos++;
        return true;
    }

    bool startsStatement(TokenType type) const {
        return type == READ_KW || type == WRITE_KW || type == IF_KW ||
               type == REPEAT_KW || type == ID;
    }

    bool isStopToken(const std::vector<TokenType>& stopTokens) const {
        for (std::size_t i = 0; i < stopTokens.size(); i++) {
            if (check(stopTokens[i])) {
                return true;
            }
        }
        return false;
    }

    void reportError(const std::string& message) {
        if (hasError) {
            return;
        }

        Token token = current();
        std::cerr << "Error at " << token.line << ":" << token.col
                  << " - " << message;
        if (!token.lexeme.empty()) {
            std::cerr << " (found: '" << token.lexeme << "')";
        }
        std::cerr << "\n";
        hasError = true;
    }

    void expect(TokenType type, const std::string& message) {
        if (!match(type)) {
            reportError(message);
        }
    }

    void program() {
        stmtSequence(std::vector<TokenType>(1, EOF_TOKEN));
    }

    void stmtSequence(const std::vector<TokenType>& stopTokens) {
        if (isAtEnd() || isStopToken(stopTokens)) {
            reportError("Expected statement");
            return;
        }

        statement();

        while (!hasError && match(SEMICOLON)) {
            if (isAtEnd() || isStopToken(stopTokens)) {
                return;
            }
            statement();
        }

        if (hasError || isAtEnd() || isStopToken(stopTokens)) {
            return;
        }

        if (startsStatement(current().type)) {
            reportError("Expected ';' between statements");
        } else {
            reportError("Unexpected token after statement");
        }
    }

    void statement() {
        if (match(READ_KW)) {
            readStatement();
            return;
        }

        if (match(WRITE_KW)) {
            writeStatement();
            return;
        }

        if (match(IF_KW)) {
            ifStatement();
            return;
        }

        if (match(REPEAT_KW)) {
            repeatStatement();
            return;
        }

        if (match(ID)) {
            assignStatement();
            return;
        }

        reportError("Expected statement");
    }

    void readStatement() {
        expect(ID, "Expected identifier after 'read'");
    }

    void writeStatement() {
        expr();
        while (!hasError && match(COMMA)) {
            expr();
        }
    }

    void ifStatement() {
        expr();
        expect(THEN_KW, "Expected 'then' after condition");
        if (hasError) {
            return;
        }

        stmtSequence(std::vector<TokenType>{ELSE_KW, END_KW});

        if (!hasError && match(ELSE_KW)) {
            stmtSequence(std::vector<TokenType>(1, END_KW));
        }

        if (!hasError) {
            expect(END_KW, "Expected 'end'");
        }
    }

    void repeatStatement() {
        stmtSequence(std::vector<TokenType>(1, UNTIL_KW));
        expect(UNTIL_KW, "Expected 'until' after repeat body");
        if (!hasError) {
            expr();
        }
    }

    void assignStatement() {
        expect(ASSIGNMENTOP, "Expected ':=' for assignment");
        if (!hasError) {
            expr();
        }
    }

    void expr() {
        simpleExpr();
        if (!hasError && match(COMPARISONOP)) {
            simpleExpr();
        }
    }

    void simpleExpr() {
        term();
        while (!hasError && (match(ADDOP) || match(SUBOP))) {
            term();
        }
    }

    void term() {
        factor();
        while (!hasError && (match(MULOP) || match(DIVOP))) {
            factor();
        }
    }

    void factor() {
        if (match(ID) || match(NUM) || match(STRING)) {
            return;
        }

        if (check(PUNCTUATION) && current().lexeme == "(") {
            pos++;
            expr();
            if (!hasError && !(check(PUNCTUATION) && current().lexeme == ")")) {
                reportError("Expected ')'");
                return;
            }
            if (!hasError) {
                pos++;
            }
            return;
        }

        reportError("Expected identifier, number, string, or '('");
    }
};

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

void printLexicalError(const Token& token) {
    std::cerr << "Error at " << token.line << ":" << token.col
              << " - " << token.message;
    if (!token.lexeme.empty()) {
        std::cerr << " (found: '" << token.lexeme << "')";
    }
    std::cerr << "\n";
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
        if (tokens[i].type == ERROR_TOKEN) {
            printLexicalError(tokens[i]);
            std::cout << "Lexical error(s) found\n";
            return 1;
        }
    }

    Parser parser(tokens);
    std::string result = parser.parse();
    std::cout << result << "\n";

    if (result == "OK - Valid Tiny program") {
        return 0;
    }

    return 1;
}
