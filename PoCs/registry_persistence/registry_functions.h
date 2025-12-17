#ifndef REGISTRY_FUNCTIONS_H
#define REGISTRY_FUNCTIONS_H

#include <windows.h>
#include <string>

bool write_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName, const std::string& data);
bool remove_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName);
bool modify_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName, const std::string& newData);

#endif