#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>

#include "..\\tiny_core.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

enum {
    ID_BTN_OPEN = 1001,
    ID_BTN_SAVE = 1002,
    ID_BTN_SCAN = 1003,
    ID_BTN_CLEAR = 1004,
    ID_EDIT_SOURCE = 2001,
    ID_LIST_TOKENS = 2002,
    ID_STATIC_STATUS = 2003,
    ID_EDIT_GRAMMAR = 2004
};

HWND gEdit = NULL;
HWND gList = NULL;
HWND gStatus = NULL;
HWND gBtnOpen = NULL;
HWND gBtnSave = NULL;
HWND gBtnScan = NULL;
HWND gBtnClear = NULL;
HFONT gFont = NULL;
HWND gGrammar = NULL;

std::string getEditText(HWND hEdit) {
    int len = GetWindowTextLengthA(hEdit);
    if (len <= 0) {
        return "";
    }

    std::vector<char> buffer(static_cast<std::size_t>(len) + 1, '\0');
    GetWindowTextA(hEdit, &buffer[0], len + 1);
    return std::string(&buffer[0]);
}

void setStatus(const std::string& s) {
    SetWindowTextA(gStatus, s.c_str());
}

void setGrammarOutput(const std::string& s) {
    SetWindowTextA(gGrammar, s.c_str());
}

std::string trimTrailingNewlines(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }
    return text;
}

std::string buildCombinedOutput(const std::string& grammarOut) {
    std::ostringstream oss;
    oss << "===== STANDARD TINY ANALYSIS =====";
    oss << "\r\n" << trimTrailingNewlines(grammarOut);
    return oss.str();
}

std::vector<Token> buildParserTokens(const std::vector<Token>& tokens) {
    std::vector<Token> parserTokens;
    for (std::size_t i = 0; i < tokens.size(); i++) {
        if (!isErrorToken(tokens[i])) {
            parserTokens.push_back(tokens[i]);
        }
    }

    if (parserTokens.empty() || parserTokens.back().type != EOF_TOKEN) {
        parserTokens.push_back(Token(EOF_TOKEN, "", 1, 1));
    }

    return parserTokens;
}

void clearTokenList() {
    ListView_DeleteAllItems(gList);
}

int insertListItemA(HWND list, int rowIndex, const std::string& text) {
    LVITEMA item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_TEXT;
    item.iItem = rowIndex;
    item.iSubItem = 0;
    item.pszText = const_cast<char*>(text.c_str());
    return static_cast<int>(SendMessageA(list, LVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&item)));
}

void setListItemTextA(HWND list, int rowIndex, int subItem, const std::string& text) {
    LVITEMA item;
    ZeroMemory(&item, sizeof(item));
    item.iSubItem = subItem;
    item.pszText = const_cast<char*>(text.c_str());
    SendMessageA(list, LVM_SETITEMTEXTA, static_cast<WPARAM>(rowIndex), reinterpret_cast<LPARAM>(&item));
}

void insertListColumnA(HWND list, int columnIndex, int width, const char* title) {
    LVCOLUMNA col;
    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.cx = width;
    col.iSubItem = columnIndex;
    col.pszText = const_cast<char*>(title);
    SendMessageA(list, LVM_INSERTCOLUMNA, static_cast<WPARAM>(columnIndex), reinterpret_cast<LPARAM>(&col));
}

void addTokenToList(const Token& token, int index) {
    int row = insertListItemA(gList, index, std::to_string(token.line));

    setListItemTextA(gList, row, 1, std::to_string(token.col));
    setListItemTextA(gList, row, 2, tokenTypeName(token.type));
    setListItemTextA(gList, row, 3, token.lexeme);
}

int countVisibleTokens(const std::vector<Token>& tokens) {
    if (!tokens.empty() && tokens.back().type == EOF_TOKEN) {
        return static_cast<int>(tokens.size()) - 1;
    }
    return static_cast<int>(tokens.size());
}

void clearEditorSelection() {
    SendMessageA(gEdit, EM_SETSEL, 0, 0);
}

void focusErrorLocation(int line, int col) {
    if (line < 1 || col < 1) {
        return;
    }

    LRESULT lineIndex = SendMessageA(gEdit, EM_LINEINDEX, static_cast<WPARAM>(line - 1), 0);
    if (lineIndex < 0) {
        return;
    }

    int start = static_cast<int>(lineIndex);
    int lineLength = static_cast<int>(SendMessageA(gEdit, EM_LINELENGTH, static_cast<WPARAM>(start), 0));
    int offset = col - 1;
    if (offset < 0) {
        offset = 0;
    }
    if (offset > lineLength) {
        offset = lineLength;
    }

    int position = start + offset;
    SendMessageA(gEdit, EM_SETSEL, static_cast<WPARAM>(position), static_cast<LPARAM>(position + 1));
    SendMessageA(gEdit, EM_SCROLLCARET, 0, 0);
    SetFocus(gEdit);
}

std::string buildAnalysisText(const std::string& result) {
    return buildCombinedOutput(result);
}

std::string findLexicalErrorText(const std::vector<Token>& tokens, int& line, int& col) {
    for (std::size_t i = 0; i < tokens.size(); i++) {
        if (isErrorToken(tokens[i])) {
            line = tokens[i].line;
            col = tokens[i].col;
            return formatLexicalError(tokens[i]);
        }
    }

    line = 0;
    col = 0;
    return "";
}

std::string buildLexicalErrorsText(const std::vector<Token>& tokens, int& firstLine, int& firstCol) {
    std::ostringstream oss;
    bool found = false;

    for (std::size_t i = 0; i < tokens.size(); i++) {
        if (!isErrorToken(tokens[i])) {
            continue;
        }

        if (!found) {
            firstLine = tokens[i].line;
            firstCol = tokens[i].col;
            found = true;
        } else {
            oss << "\r\n";
        }

        oss << formatLexicalError(tokens[i]);
    }

    if (!found) {
        firstLine = 0;
        firstCol = 0;
        return "";
    }

    return oss.str();
}

std::string buildSyntaxErrorsTextSkippingLexicalLines(const Parser& parser,
                                                      const std::vector<Token>& tokens,
                                                      bool& hasSyntaxErrors) {
    std::set<int> lexicalLines;
    for (std::size_t i = 0; i < tokens.size(); i++) {
        if (isErrorToken(tokens[i])) {
            lexicalLines.insert(tokens[i].line);
        }
    }

    std::ostringstream oss;
    bool wroteAny = false;
    for (std::size_t i = 0; i < parser.errors.size(); i++) {
        const ParseError& error = parser.errors[i];
        if (lexicalLines.find(error.line) != lexicalLines.end()) {
            continue;
        }

        if (wroteAny) {
            oss << "\r\n";
        }

        oss << "Syntax error at line " << error.line
            << ", col " << error.col << ": " << error.message;
        if (!error.lexeme.empty()) {
            oss << " (found: '" << error.lexeme << "')";
        }

        wroteAny = true;
    }

    hasSyntaxErrors = wroteAny;
    return oss.str();
}

void runScan() {
    clearTokenList();
    setGrammarOutput("");

    std::string source = getEditText(gEdit);
    if (source.empty()) {
        setStatus("Source editor is empty.");
        return;
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanAll();

    for (int i = 0; i < static_cast<int>(tokens.size()); i++) {
        addTokenToList(tokens[i], i);
    }

    if (tokens.empty()) {
        setStatus("No tokens were produced.");
        return;
    }

    int lexicalLine = 0;
    int lexicalCol = 0;
    std::string lexicalErrors = buildLexicalErrorsText(tokens, lexicalLine, lexicalCol);
    bool hasLexicalErrors = !lexicalErrors.empty();

    std::vector<Token> parserTokens = buildParserTokens(tokens);
    Parser parser(parserTokens);
    parser.parse();
    bool hasSyntaxError = false;
    std::string syntaxError = buildSyntaxErrorsTextSkippingLexicalLines(parser, tokens, hasSyntaxError);

    std::ostringstream analysis;
    if (hasLexicalErrors && hasSyntaxError) {
        analysis << "[Lexical]\r\n" << lexicalErrors << "\r\n\r\n";
        analysis << "[Syntax]\r\n" << syntaxError;
        setStatus("Lexical and syntax errors found.");
        focusErrorLocation(lexicalLine, lexicalCol);
    } else if (hasLexicalErrors) {
        analysis << "[Lexical]\r\n" << lexicalErrors;
        setStatus("Lexical error(s) found.");
        focusErrorLocation(lexicalLine, lexicalCol);
    } else if (hasSyntaxError) {
        analysis << "[Syntax]\r\n" << syntaxError;
        setStatus("Syntax error(s) found.");
        focusErrorLocation(parser.errorLine, parser.errorCol);
    } else {
        clearEditorSelection();
        analysis << "OK - Valid Tiny program";
        std::ostringstream status;
        status << "Valid Tiny program. Tokens: " << countVisibleTokens(tokens);
        setStatus(status.str());
    }

    setGrammarOutput(buildAnalysisText(analysis.str()));
}

void openFileDialog(HWND hWnd) {
    char fileName[MAX_PATH] = {0};

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Tiny Files (*.tiny)\0*.tiny\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameA(&ofn)) {
        return;
    }

    std::ifstream in(fileName, std::ios::binary);
    if (!in) {
        setStatus("Could not open selected file.");
        return;
    }

    std::string content;
    char ch;
    while (in.get(ch)) {
        content.push_back(ch);
    }

    SetWindowTextA(gEdit, content.c_str());
    setStatus(std::string("Loaded: ") + fileName);
}

void saveFileDialog(HWND hWnd) {
    char fileName[MAX_PATH] = {0};

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Tiny Files (*.tiny)\0*.tiny\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "tiny";

    if (!GetSaveFileNameA(&ofn)) {
        return;
    }

    std::string text = getEditText(gEdit);
    std::ofstream out(fileName, std::ios::binary);
    if (!out) {
        setStatus("Could not save file.");
        return;
    }

    out << text;
    setStatus(std::string("Saved: ") + fileName);
}

void applyFont(HWND hWnd) {
    SendMessageA(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(gFont), TRUE);
}

void createControls(HWND hWnd) {
    gBtnOpen = CreateWindowA("BUTTON", "Open", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                             10, 10, 90, 30, hWnd, (HMENU)ID_BTN_OPEN, NULL, NULL);
    gBtnSave = CreateWindowA("BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                             110, 10, 90, 30, hWnd, (HMENU)ID_BTN_SAVE, NULL, NULL);
    gBtnScan = CreateWindowA("BUTTON", "Scan", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                             210, 10, 90, 30, hWnd, (HMENU)ID_BTN_SCAN, NULL, NULL);
    gBtnClear = CreateWindowA("BUTTON", "Clear", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              310, 10, 90, 30, hWnd, (HMENU)ID_BTN_CLEAR, NULL, NULL);

    gEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
                            WS_VSCROLL | WS_HSCROLL,
                            10, 50, 430, 400, hWnd, (HMENU)ID_EDIT_SOURCE, NULL, NULL);

    gList = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
                            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                            450, 50, 520, 400, hWnd, (HMENU)ID_LIST_TOKENS, NULL, NULL);
    gGrammar = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                               WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL |
                               ES_READONLY | WS_VSCROLL,
                               10, 370, 960, 170, hWnd, (HMENU)ID_EDIT_GRAMMAR, NULL, NULL);

    gStatus = CreateWindowA("STATIC", "Ready. Normal Tiny grammar.", WS_CHILD | WS_VISIBLE | SS_LEFT,
                            10, 550, 960, 24, hWnd, (HMENU)ID_STATIC_STATUS, NULL, NULL);

    insertListColumnA(gList, 0, 60, "Line");
    insertListColumnA(gList, 1, 60, "Col");
    insertListColumnA(gList, 2, 170, "Type");
    insertListColumnA(gList, 3, 220, "Lexeme");

    gFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    applyFont(gBtnOpen);
    applyFont(gBtnSave);
    applyFont(gBtnScan);
    applyFont(gBtnClear);
    applyFont(gEdit);
    applyFont(gList);
    applyFont(gGrammar);
    applyFont(gStatus);
}

void resizeControls(HWND hWnd) {
    RECT r;
    GetClientRect(hWnd, &r);

    int width = r.right - r.left;
    int height = r.bottom - r.top;

    int margin = 10;
    int topBarH = 30;
    int grammarH = 170;
    int statusH = 26;
    int contentTop = margin + topBarH + 10;
    int contentBottom = height - margin - statusH - grammarH - 20;
    int contentH = contentBottom - contentTop;

    int leftW = (width - 3 * margin) * 45 / 100;
    int rightW = width - 3 * margin - leftW;

    MoveWindow(gBtnOpen, margin, margin, 90, topBarH, TRUE);
    MoveWindow(gBtnSave, margin + 100, margin, 90, topBarH, TRUE);
    MoveWindow(gBtnScan, margin + 200, margin, 90, topBarH, TRUE);
    MoveWindow(gBtnClear, margin + 300, margin, 90, topBarH, TRUE);

    MoveWindow(gEdit, margin, contentTop, leftW, contentH, TRUE);
    MoveWindow(gList, margin + leftW + margin, contentTop, rightW, contentH, TRUE);
    MoveWindow(gGrammar, margin, contentBottom + 10, width - 2 * margin, grammarH, TRUE);
    MoveWindow(gStatus, margin, height - margin - statusH, width - 2 * margin, statusH, TRUE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            createControls(hWnd);
            return 0;

        case WM_SIZE:
            resizeControls(hWnd);
            return 0;

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_OPEN) {
                openFileDialog(hWnd);
            } else if (id == ID_BTN_SAVE) {
                saveFileDialog(hWnd);
            } else if (id == ID_BTN_SCAN) {
                runScan();
            } else if (id == ID_BTN_CLEAR) {
                SetWindowTextA(gEdit, "");
                clearTokenList();
                setGrammarOutput("");
                setStatus("Cleared.");
            }
            return 0;
        }

        case WM_DESTROY:
            if (gFont) {
                DeleteObject(gFont);
                gFont = NULL;
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "TinyScannerGuiClass";

    RegisterClassA(&wc);

    HWND hWnd = CreateWindowA(
        "TinyScannerGuiClass",
        "Tiny Compiler Scanner GUI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1100,
        650,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd) {
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
