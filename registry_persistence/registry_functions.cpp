#include <windows.h>
#include <string>

bool write_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName, const std::string& data) {
    HKEY hKey;
    if (RegCreateKeyExA(hKeyRoot, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }
    
    bool result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ, (BYTE*)data.c_str(), data.length() + 1) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return result;
}

bool remove_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName) {
    HKEY hKey;
    if (RegOpenKeyExA(hKeyRoot, subKey.c_str(), 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    bool result = RegDeleteValueA(hKey, valueName.c_str()) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return result;
}

bool modify_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName, const std::string& newData) {
    HKEY hKey;
    if (RegOpenKeyExA(hKeyRoot, subKey.c_str(), 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    bool result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ, (BYTE*)newData.c_str(), newData.length() + 1) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return result;
}