#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>

#include <cstdio>
#include <fstream>
#include <sstream>
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

struct ParsedToken {
    std::string line;
    std::string col;
    std::string type;
    std::string lexeme;
};

bool fileExists(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::string getExeDir() {
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return ".";
    }

    std::string full(path);
    std::size_t pos = full.find_last_of("\\/");
    if (pos == std::string::npos) {
        return ".";
    }
    return full.substr(0, pos);
}

std::string findScannerPath() {
    const std::string exeName = "scanner.exe";
    if (fileExists(exeName)) {
        return ".\\" + exeName;
    }
    if (fileExists("..\\" + exeName)) {
        return "..\\" + exeName;
    }

    std::string exeDir = getExeDir();
    std::string sameDir = exeDir + "\\" + exeName;
    if (fileExists(sameDir)) {
        return sameDir;
    }

    std::string parentDir = exeDir + "\\..\\" + exeName;
    if (fileExists(parentDir)) {
        return parentDir;
    }

    return "";
}

std::string findGrammarPath() {
    const std::string exeName = "grammar.exe";
    if (fileExists(exeName)) {
        return ".\\" + exeName;
    }
    if (fileExists("..\\" + exeName)) {
        return "..\\" + exeName;
    }

    std::string exeDir = getExeDir();
    std::string sameDir = exeDir + "\\" + exeName;
    if (fileExists(sameDir)) {
        return sameDir;
    }

    std::string parentDir = exeDir + "\\..\\" + exeName;
    if (fileExists(parentDir)) {
        return parentDir;
    }

    return "";
}

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
    oss << "===== GRAMMAR OUTPUT =====";
    oss << "\r\n" << trimTrailingNewlines(grammarOut);
    return oss.str();
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

void addTokenToList(const ParsedToken& t, int index) {
    int row = insertListItemA(gList, index, t.line);

    setListItemTextA(gList, row, 1, t.col);
    setListItemTextA(gList, row, 2, t.type);
    setListItemTextA(gList, row, 3, t.lexeme);
}

bool parseTokenLine(const std::string& line, ParsedToken& out) {
    std::size_t p1 = line.find("  ");
    if (p1 == std::string::npos) {
        return false;
    }

    std::size_t p2 = line.find("  ", p1 + 2);
    if (p2 == std::string::npos) {
        p2 = line.size();
    }

    std::string loc = line.substr(0, p1);
    std::string type = line.substr(p1 + 2, p2 - (p1 + 2));
    std::string lexeme = "";
    if (p2 + 2 <= line.size()) {
        lexeme = line.substr(p2 + 2);
    }

    std::size_t colon = loc.find(':');
    if (colon == std::string::npos) {
        return false;
    }

    out.line = loc.substr(0, colon);
    out.col = loc.substr(colon + 1);
    out.type = type;
    out.lexeme = lexeme;
    return true;
}

bool writeTempInput(const std::string& text, std::string& tempPathOut) {
    char tempDir[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, tempDir);
    if (len == 0 || len > MAX_PATH) {
        return false;
    }

    tempPathOut = std::string(tempDir) + "tiny_gui_input.tiny";
    std::ofstream out(tempPathOut.c_str(), std::ios::binary);
    if (!out) {
        return false;
    }
    out << text;
    return true;
}

bool runProcessCapture(const std::string& exePath, const std::string& inputPath, std::string& output, int& exitCode) {
    std::string command = "cmd /C \"\"" + exePath + "\" \"" + inputPath + "\" 2>&1\"";
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        return false;
    }

    output.clear();
    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        output += buffer;
    }

    exitCode = _pclose(pipe);
    return true;
}

void runScan() {
    clearTokenList();
    setGrammarOutput("");

    std::string source = getEditText(gEdit);
    if (source.empty()) {
        setStatus("Source editor is empty.");
        return;
    }

    std::string scannerPath = findScannerPath();
    if (scannerPath.empty()) {
        setStatus("scanner.exe not found. Build scanner first.");
        return;
    }
    std::string grammarPath = findGrammarPath();
    if (grammarPath.empty()) {
        setStatus("grammar.exe not found. Build grammar first.");
        return;
    }

    std::string tempPath;
    if (!writeTempInput(source, tempPath)) {
        setStatus("Failed to write temp input file.");
        return;
    }

    std::string grammarOut;
    int grammarExitCode = 0;
    if (!runProcessCapture(grammarPath, tempPath, grammarOut, grammarExitCode)) {
        setStatus("Failed to run grammar process.");
        DeleteFileA(tempPath.c_str());
        return;
    }

    if (grammarOut.empty()) {
        grammarOut = "(no grammar output)";
    }

    setGrammarOutput(buildCombinedOutput(grammarOut));
    RedrawWindow(gGrammar, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    std::string scanOut;
    int scanExitCode = 0;
    if (!runProcessCapture(scannerPath, tempPath, scanOut, scanExitCode)) {
        std::ostringstream oss;
        oss << "Grammar shown. Failed to run scanner process.";
        if (grammarExitCode != 0) {
            oss << " (grammar exit code: " << grammarExitCode << ")";
        }
        setStatus(oss.str());
        DeleteFileA(tempPath.c_str());
        return;
    }
    std::vector<ParsedToken> tokens;
    bool parseError = false;
    std::istringstream scanStream(scanOut);
    std::string line;
    while (std::getline(scanStream, line)) {
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        ParsedToken t;
        if (parseTokenLine(line, t)) {
            tokens.push_back(t);
        } else {
            parseError = true;
        }
    }

    for (int i = 0; i < static_cast<int>(tokens.size()); i++) {
        addTokenToList(tokens[i], i);
    }

    if (scanExitCode == -1) {
        std::ostringstream oss;
        oss << "Grammar shown. Failed to close scanner process.";
        if (grammarExitCode != 0) {
            oss << " (grammar exit code: " << grammarExitCode << ")";
        }
        setStatus(oss.str());
        DeleteFileA(tempPath.c_str());
        return;
    }

    if (scanExitCode != 0) {
        std::ostringstream oss;
        oss << "Grammar shown. Scanner returned a non-zero exit code.";
        if (grammarExitCode != 0) {
            oss << " (grammar exit code: " << grammarExitCode << ")";
        }
        setStatus(oss.str());
        DeleteFileA(tempPath.c_str());
        return;
    }

    if (tokens.empty()) {
        std::ostringstream oss;
        oss << "Grammar shown. Scanner produced no output.";
        if (grammarExitCode != 0) {
            oss << " (grammar exit code: " << grammarExitCode << ")";
        }
        setStatus(oss.str());
        DeleteFileA(tempPath.c_str());
        return;
    }

    for (std::size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == "ERROR") {
            std::ostringstream oss;
            oss << "Grammar shown. Lexical error at line " << tokens[i].line
                << ", col " << tokens[i].col << ".";
            if (grammarExitCode != 0) {
                oss << " (grammar exit code: " << grammarExitCode << ")";
            }
            setStatus(oss.str());
            DeleteFileA(tempPath.c_str());
            return;
        }
    }

    if (parseError) {
        std::ostringstream oss;
        oss << "Grammar shown. Scan done, but some output lines could not be parsed.";
        if (grammarExitCode != 0) {
            oss << " (grammar exit code: " << grammarExitCode << ")";
        }
        setStatus(oss.str());
        DeleteFileA(tempPath.c_str());
        return;
    }

    DeleteFileA(tempPath.c_str());

    std::ostringstream oss;
    oss << "Grammar shown. Scan complete. Tokens: " << tokens.size();
    if (grammarExitCode != 0) {
        oss << " (grammar exit code: " << grammarExitCode << ")";
    }
    setStatus(oss.str());
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
                               10, 460, 960, 80, hWnd, (HMENU)ID_EDIT_GRAMMAR, NULL, NULL);

    gStatus = CreateWindowA("STATIC", "Ready.", WS_CHILD | WS_VISIBLE | SS_LEFT,
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
    int grammarH = 80;
    int statusH = 24;
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
