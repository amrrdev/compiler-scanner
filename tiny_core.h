#ifndef TINY_CORE_H
#define TINY_CORE_H

#include <cctype>
#include <fstream>
#include <string>
#include <vector>

enum TinyTokenType {
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
    ADDOP,
    SUBOP,
    MULOP,
    DIVOP,
    COMPARISONOP,
    ASSIGNMENTOP,
    SEMICOLON,
    PUNCTUATION,
    EOF_TOKEN,
    ERROR_TOKEN
};

struct Token {
    TinyTokenType type;
    std::string lexeme;
    std::string message;
    int line;
    int col;

    Token()
        : type(ERROR_TOKEN), lexeme(""), message(""), line(0), col(0) {}

    Token(TinyTokenType tokenType, const std::string& tokenLexeme, int tokenLine, int tokenCol,
          const std::string& tokenMessage = "")
        : type(tokenType),
          lexeme(tokenLexeme),
          message(tokenMessage),
          line(tokenLine),
          col(tokenCol) {}
};

struct ParseError {
    std::string message;
    int line;
    int col;
    std::string lexeme;

    ParseError() : message(""), line(0), col(0), lexeme("") {}

    ParseError(const std::string& errorMessage, int errorLine, int errorCol,
               const std::string& errorLexeme)
        : message(errorMessage), line(errorLine), col(errorCol), lexeme(errorLexeme) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source)
        : source_(source), pos_(0), line_(1), col_(1) {}

    std::vector<Token> scanAll() {
        std::vector<Token> tokens;

        while (true) {
            Token token = nextToken();
            tokens.push_back(token);

            if (token.type == EOF_TOKEN) {
                break;
            }
        }

        return tokens;
    }

private:
    std::string source_;
    std::size_t pos_;
    int line_;
    int col_;

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

        char ch = source_[pos_];
        pos_++;

        if (ch == '\n') {
            line_++;
            col_ = 1;
        } else {
            col_++;
        }

        return ch;
    }

    bool isLetter(char ch) const {
        return std::isalpha(static_cast<unsigned char>(ch)) != 0;
    }

    bool isDigit(char ch) const {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    }

    Token makeToken(TinyTokenType type, const std::string& lexeme, int line, int col,
                    const std::string& message = "") const {
        return Token(type, lexeme, line, col, message);
    }

    TinyTokenType keywordType(const std::string& word) const {
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

    std::string invalidCharacterMessage(char ch) const {
        if (ch == ':') {
            return "Expected '=' after ':'";
        }
        if (ch == '"') {
            return "Strings are not part of normal Tiny grammar";
        }
        if (ch == ',') {
            return "Commas are not part of normal Tiny grammar";
        }
        if (ch == '>' || ch == '!') {
            return "Only '<' and '=' are valid comparison operators in normal Tiny grammar";
        }
        return "Invalid character";
    }

    bool skipIgnoredText(Token& errorToken) {
        while (!isAtEnd()) {
            char ch = peek();

            if (std::isspace(static_cast<unsigned char>(ch))) {
                advance();
                continue;
            }

            if (ch == '{') {
                int startLine = line_;
                int startCol = col_;
                advance();

                while (!isAtEnd() && peek() != '}') {
                    if (peek() == '{') {
                        errorToken = makeToken(ERROR_TOKEN, "{", startLine, startCol,
                                               "Nested comments are not allowed");
                        return false;
                    }
                    advance();
                }

                if (isAtEnd()) {
                    errorToken = makeToken(ERROR_TOKEN, "{", startLine, startCol,
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
            return makeToken(EOF_TOKEN, "", line_, col_);
        }

        int startLine = line_;
        int startCol = col_;
        char ch = peek();

        if (isLetter(ch)) {
            return scanIdentifier();
        }

        if (isDigit(ch)) {
            return scanNumber();
        }

        if (ch == ':' && peekNext() == '=') {
            advance();
            advance();
            return makeToken(ASSIGNMENTOP, ":=", startLine, startCol);
        }

        advance();

        if (ch == '+') return makeToken(ADDOP, "+", startLine, startCol);
        if (ch == '-') return makeToken(SUBOP, "-", startLine, startCol);
        if (ch == '*') return makeToken(MULOP, "*", startLine, startCol);
        if (ch == '/') return makeToken(DIVOP, "/", startLine, startCol);
        if (ch == '=' || ch == '<') return makeToken(COMPARISONOP, std::string(1, ch), startLine, startCol);
        if (ch == ';') return makeToken(SEMICOLON, ";", startLine, startCol);
        if (ch == '(' || ch == ')') return makeToken(PUNCTUATION, std::string(1, ch), startLine, startCol);

        return makeToken(ERROR_TOKEN, std::string(1, ch), startLine, startCol,
                         invalidCharacterMessage(ch));
    }
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens)
        : tokens_(tokens),
          pos_(0),
          hasError(false),
          errorMessage(""),
          errorLine(0),
          errorCol(0),
          errorLexeme("") {}

    bool parse() {
        hasError = false;
        errorMessage.clear();
        errorLine = 0;
        errorCol = 0;
        errorLexeme.clear();
        errors.clear();
        pos_ = 0;

        program();

        if (!isAtEnd()) {
            reportError("Unexpected token after statement");
        }

        return !hasError;
    }

    bool hasError;
    std::string errorMessage;
    int errorLine;
    int errorCol;
    std::string errorLexeme;
    std::vector<ParseError> errors;

private:
    const std::vector<Token>& tokens_;
    std::size_t pos_;

    Token current() const {
        if (pos_ < tokens_.size()) {
            return tokens_[pos_];
        }
        return Token(EOF_TOKEN, "", 0, 0);
    }

    bool isAtEnd() const {
        return current().type == EOF_TOKEN;
    }

    bool check(TinyTokenType type) const {
        return current().type == type;
    }

    bool match(TinyTokenType type) {
        if (!check(type)) {
            return false;
        }

        pos_++;
        return true;
    }

    bool startsStatement(TinyTokenType type) const {
        return type == READ_KW || type == WRITE_KW || type == IF_KW ||
               type == REPEAT_KW || type == ID;
    }

    bool isStopToken(const std::vector<TinyTokenType>& stopTokens) const {
        for (std::size_t i = 0; i < stopTokens.size(); i++) {
            if (check(stopTokens[i])) {
                return true;
            }
        }
        return false;
    }

    void synchronizeToStatement(const std::vector<TinyTokenType>& stopTokens) {
        while (!isAtEnd()) {
            if (check(SEMICOLON) || isStopToken(stopTokens)) {
                return;
            }
            pos_++;
        }
    }

    void recoverStatement(const std::vector<TinyTokenType>& stopTokens) {
        synchronizeToStatement(stopTokens);
        if (check(SEMICOLON)) {
            pos_++;
        }
    }

    void reportError(const std::string& message) {
        Token token = current();
        ParseError error(message, token.line, token.col, token.lexeme);

        if (!errors.empty()) {
            const ParseError& last = errors.back();
            if (last.line == error.line && last.col == error.col && last.message == error.message) {
                return;
            }
        }

        errors.push_back(error);
        hasError = true;

        if (errorMessage.empty()) {
            errorMessage = message;
            errorLine = token.line;
            errorCol = token.col;
            errorLexeme = token.lexeme;
        }
    }

    void expect(TinyTokenType type, const std::string& message) {
        if (!match(type)) {
            reportError(message);
        }
    }

    void program() {
        stmtSequence(std::vector<TinyTokenType>(1, EOF_TOKEN));
    }

    void stmtSequence(const std::vector<TinyTokenType>& stopTokens) {
        if (isAtEnd() || isStopToken(stopTokens)) {
            reportError("Expected statement");
            return;
        }

        while (!isAtEnd() && !isStopToken(stopTokens)) {
            bool statementOk = statement(stopTokens);

            if (!statementOk) {
                continue;
            }

            if (match(SEMICOLON)) {
                if (isAtEnd() || isStopToken(stopTokens)) {
                    reportError("Expected statement after ';'");
                    return;
                }
                continue;
            }

            if (isAtEnd() || isStopToken(stopTokens)) {
                return;
            }

            if (startsStatement(current().type)) {
                reportError("Missing ';' before '" + current().lexeme + "'");
                continue;
            }

            reportError("Unexpected token after statement");
            recoverStatement(stopTokens);
        }
    }

    bool statement(const std::vector<TinyTokenType>& stopTokens) {
        if (match(READ_KW)) {
            return readStatement(stopTokens);
        }

        if (match(WRITE_KW)) {
            return writeStatement(stopTokens);
        }

        if (match(IF_KW)) {
            return ifStatement(stopTokens);
        }

        if (match(REPEAT_KW)) {
            return repeatStatement(stopTokens);
        }

        if (match(ID)) {
            return assignStatement(stopTokens);
        }

        reportError("Expected statement");
        recoverStatement(stopTokens);
        return false;
    }

    bool readStatement(const std::vector<TinyTokenType>& stopTokens) {
        if (match(ID)) {
            return true;
        }

        reportError("Expected identifier after 'read'");
        recoverStatement(stopTokens);
        return false;
    }

    bool writeStatement(const std::vector<TinyTokenType>& stopTokens) {
        if (check(SEMICOLON) || isAtEnd() || isStopToken(stopTokens)) {
            reportError("Expected expression after 'write'");
            recoverStatement(stopTokens);
            return false;
        }

        if (!expr()) {
            recoverStatement(stopTokens);
            return false;
        }

        return true;
    }

    bool ifStatement(const std::vector<TinyTokenType>& stopTokens) {
        std::size_t errorCountBefore = errors.size();

        if (!expr()) {
            reportError("Expected condition expression after 'if'");
            recoverStatement(stopTokens);
            return false;
        }

        if (!match(THEN_KW)) {
            reportError("Expected 'then' after condition");
            recoverStatement(stopTokens);
            return false;
        }

        stmtSequence(std::vector<TinyTokenType>{ELSE_KW, END_KW});

        if (match(ELSE_KW)) {
            stmtSequence(std::vector<TinyTokenType>(1, END_KW));
        }

        if (!match(END_KW)) {
            reportError("Expected 'end'");
            recoverStatement(stopTokens);
            return false;
        }

        return errors.size() == errorCountBefore;
    }

    bool repeatStatement(const std::vector<TinyTokenType>& stopTokens) {
        std::size_t errorCountBefore = errors.size();

        stmtSequence(std::vector<TinyTokenType>(1, UNTIL_KW));

        if (!match(UNTIL_KW)) {
            reportError("Expected 'until' after repeat body");
            recoverStatement(stopTokens);
            return false;
        }

        if (!expr()) {
            reportError("Expected expression after 'until'");
            recoverStatement(stopTokens);
            return false;
        }

        return errors.size() == errorCountBefore;
    }

    bool assignStatement(const std::vector<TinyTokenType>& stopTokens) {
        if (!match(ASSIGNMENTOP)) {
            reportError("Expected ':=' for assignment");
            recoverStatement(stopTokens);
            return false;
        }

        if (check(SEMICOLON) || isAtEnd() || isStopToken(stopTokens)) {
            reportError("Expected expression after ':='");
            recoverStatement(stopTokens);
            return false;
        }

        if (!expr()) {
            recoverStatement(stopTokens);
            return false;
        }

        return true;
    }

    bool expr() {
        if (!simpleExpr()) {
            return false;
        }
        if (match(COMPARISONOP)) {
            if (!simpleExpr()) {
                return false;
            }
        }
        return true;
    }

    bool simpleExpr() {
        if (!term()) {
            return false;
        }

        while (match(ADDOP) || match(SUBOP)) {
            if (!term()) {
                return false;
            }
        }

        return true;
    }

    bool term() {
        if (!factor()) {
            return false;
        }

        while (match(MULOP) || match(DIVOP)) {
            if (!factor()) {
                return false;
            }
        }

        return true;
    }

    bool factor() {
        if (match(ID) || match(NUM)) {
            return true;
        }

        if (check(PUNCTUATION) && current().lexeme == "(") {
            pos_++;
            if (!expr()) {
                return false;
            }
            if (!(check(PUNCTUATION) && current().lexeme == ")")) {
                reportError("Expected ')'");
                return false;
            }
            pos_++;
            return true;
        }

        reportError("Expected identifier, number, or '('");
        return false;
    }
};

inline bool isErrorToken(const Token& token) {
    return token.type == ERROR_TOKEN;
}

inline std::string tokenTypeName(TinyTokenType type) {
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
    if (type == ADDOP) return "ADDOP";
    if (type == SUBOP) return "SUBOP";
    if (type == MULOP) return "MULOP";
    if (type == DIVOP) return "DIVOP";
    if (type == COMPARISONOP) return "COMPARISONOP";
    if (type == ASSIGNMENTOP) return "ASSIGNMENTOP";
    if (type == SEMICOLON) return "SEMICOLON";
    if (type == PUNCTUATION) return "PUNCTUATION";
    if (type == EOF_TOKEN) return "EOF";
    return "ERROR";
}

inline std::string formatLexicalError(const Token& token) {
    std::string text = "Lexical error at line " + std::to_string(token.line) +
                       ", col " + std::to_string(token.col) + ": " + token.message;
    if (!token.lexeme.empty()) {
        text += " (found: '" + token.lexeme + "')";
    }
    return text;
}

inline std::string formatSyntaxError(const Parser& parser) {
    std::string text = "Syntax error at line " + std::to_string(parser.errorLine) +
                       ", col " + std::to_string(parser.errorCol) + ": " + parser.errorMessage;
    if (!parser.errorLexeme.empty()) {
        text += " (found: '" + parser.errorLexeme + "')";
    }
    return text;
}

inline std::string formatSyntaxErrors(const Parser& parser) {
    if (parser.errors.empty()) {
        return "";
    }

    std::string text;
    for (std::size_t i = 0; i < parser.errors.size(); i++) {
        const ParseError& error = parser.errors[i];
        if (i != 0) {
            text += "\r\n";
        }

        text += "Syntax error at line " + std::to_string(error.line) +
                ", col " + std::to_string(error.col) + ": " + error.message;
        if (!error.lexeme.empty()) {
            text += " (found: '" + error.lexeme + "')";
        }
    }

    return text;
}

inline std::string readTextFile(const std::string& path) {
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

#endif
