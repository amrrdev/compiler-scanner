# Tiny Language Scanner (Lexer)

This project is the **first phase of a compiler**.

It reads a Tiny-language source file and turns it into a stream of **tokens**.

---

## 1) What is a compiler?

A compiler is a program that translates code from one language (source language) into another form (target language).

Typical compiler phases:

1. **Lexical analysis (scanner / lexer)**
2. Syntax analysis (parser)
3. Semantic analysis
4. Intermediate code generation
5. Optimization
6. Target code generation

This repository currently implements only **phase 1: lexical analysis**.

---

## 2) What is a scanner (lexer)?

The scanner reads text characters and groups them into meaningful pieces called **lexemes**.

Each lexeme is converted into a **token**.

Example:

Input text:

```txt
if x<10 then
```

Token stream:

- `IF_KW` with lexeme `if`
- `ID` with lexeme `x`
- `COMPARISONOP` with lexeme `<`
- `NUM` with lexeme `10`
- `THEN_KW` with lexeme `then`

The scanner also:

- skips whitespace
- skips comments
- tracks line/column for error reporting

---

## 3) Tiny language rules used here

This scanner follows your Tiny language description:

- **Reserved words**:
  - `if`, `then`, `else`, `end`, `repeat`, `until`, `read`, `write`
- **Identifiers**:
  - start with a letter, then letters/digits
- **Numbers**:
  - integer only (sequence of digits)
- **Strings**:
  - inside double quotes `"..."` (used in write)
- **Operators / symbols**:
  - `+ - * /`
  - comparison: `=` and `<`
  - assignment: `:=`
  - separators: `;` and `,`
  - punctuation: `(` and `)`
- **Comments**:
  - inside curly braces: `{ ... }`
  - nested comments are treated as an error

---

## 4) Project files

- `scanner.cpp` - complete scanner implementation
- `sample_input.tiny` - sample Tiny program (factorial)

---

## 5) How the code is structured (simple classes)

### `enum TokenType`

Defines all token categories, for example:

- `IF_KW`, `READ_KW`, `ID`, `NUM`, `ASSIGNMENTOP`, `EOF_TOKEN`, `ERROR_TOKEN`

### `class Token`

Represents one token with:

- `type` - token category
- `lexeme` - exact text from source
- `line` and `col` - position in input

### `class Lexer`

Main scanner class. It stores source text and current position, then provides:

- `scanAll()` - scans entire file and returns all tokens
- helper methods like:
  - `peek()`, `peekNext()`, `advance()`
  - `scanIdentifier()`, `scanNumber()`, `scanString()`
  - `skipSpacesAndComments()`
  - `nextToken()`

### `main()`

Program entry point:

1. Reads file path from command line
2. Loads file text
3. Creates `Lexer`
4. Prints token stream line by line

---

## 6) How to compile and run

## Windows PowerShell (with g++)

```powershell
g++ -std=c++17 scanner.cpp -o scanner.exe
.\scanner.exe .\sample_input.tiny
```

## GUI

```powershell
g++ -std=c++17 gui\gui.cpp -mwindows -o gui\scanner_gui.exe -lcomctl32 -lcomdlg32
.\gui\scanner_gui.exe
```

---

## 7) Example output

You will get output similar to:

```txt
2:1  READ_KW  read
2:6  ID  x
2:7  SEMICOLON  ;
3:1  IF_KW  if
3:4  NUM  0
3:5  COMPARISONOP  <
...
```

Format is:

`line:column  TOKEN_TYPE  lexeme`

---

## 8) Error handling currently supported

The scanner returns `ERROR` token for lexical problems such as:

- unknown character
- unclosed string
- unclosed comment `{ ...`
- nested comment (`{ ... { ... } ... }`)

When an error token appears, scanning stops.

---

## 9) What this project does NOT do yet

Not implemented yet:

- parser (syntax tree)
- semantic analysis
- code generation

So this project only says: "What are the tokens?"

It does **not** validate full grammar rules yet.

---

## 10) Suggested next steps

To continue building a full compiler, add these modules next:

1. Parser (for Tiny grammar)
2. AST node classes
3. Symbol table
4. Semantic checks

The current scanner class is intentionally simple so you can extend it step by step.

---

## 11) GUI (separate folder)

A C++ Windows GUI is included in `gui/gui.cpp`.

It is separate from the scanner code and does not change the scanner structure.

GUI features:

- source code editor (left)
- token table (right): `Line`, `Col`, `Type`, `Lexeme`
- buttons: `Open`, `Save`, `Scan`, `Clear`
- status line for success/errors

How it works:

1. GUI writes editor text to a temporary `.tiny` file
2. GUI runs `scanner.exe <temp-file>`
3. GUI reads scanner output and fills the token table
