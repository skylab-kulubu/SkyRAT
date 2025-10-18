#include "Utilities.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <ctime>
#include <chrono>
#include <regex>
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

namespace SkyRAT {
namespace Utils {

// === StringUtils Implementation ===

std::string StringUtils::trim(const std::string& str) {
    const char* whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string StringUtils::ltrim(const std::string& str) {
    const char* whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    return (start == std::string::npos) ? "" : str.substr(start);
}

std::string StringUtils::rtrim(const std::string& str) {
    const char* whitespace = " \t\r\n";
    size_t end = str.find_last_not_of(whitespace);
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::vector<std::string> StringUtils::split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    
    return result;
}

std::vector<std::string> StringUtils::split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    
    result.push_back(str.substr(start));
    return result;
}

std::string StringUtils::join(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    
    std::ostringstream result;
    result << strings[0];
    
    for (size_t i = 1; i < strings.size(); ++i) {
        result << delimiter << strings[i];
    }
    
    return result.str();
}

std::string StringUtils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string StringUtils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

bool StringUtils::startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

bool StringUtils::endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.substr(str.size() - suffix.size()) == suffix;
}

std::string StringUtils::replace(const std::string& str, const std::string& search, const std::string& replace) {
    std::string result = str;
    size_t pos = 0;
    
    while ((pos = result.find(search, pos)) != std::string::npos) {
        result.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    
    return result;
}

std::wstring StringUtils::stringToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string StringUtils::wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// === FileUtils Implementation ===

std::string FileUtils::extractDirectory(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return filepath.substr(0, lastSlash);
    }
    return "."; // Current directory if no slash found
}

bool FileUtils::directoryExists(const std::string& path) {
    DWORD dwAttrib = GetFileAttributesA(path.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
            (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::string FileUtils::sanitizeFileName(const std::string& filename) {
    std::string result = filename;
    
    // Remove or replace invalid characters
    const std::string invalidChars = "<>:\"/\\|?*";
    for (char& c : result) {
        if (invalidChars.find(c) != std::string::npos || c < 32) {
            c = '_';
        }
    }
    
    // Trim spaces and dots from the end
    result = StringUtils::rtrim(result);
    while (!result.empty() && result.back() == '.') {
        result.pop_back();
    }
    
    // Ensure filename is not empty and not too long
    if (result.empty()) {
        result = "file";
    }
    
    if (result.length() > 255) {
        result = result.substr(0, 255);
    }
    
    return result;
}

std::string FileUtils::getFileExtension(const std::string& filepath) {
    size_t lastDot = filepath.find_last_of('.');
    size_t lastSep = filepath.find_last_of("/\\");
    
    if (lastDot != std::string::npos && 
        (lastSep == std::string::npos || lastDot > lastSep)) {
        return filepath.substr(lastDot);
    }
    
    return "";
}

std::string FileUtils::getFileName(const std::string& filepath) {
    size_t lastSep = filepath.find_last_of("/\\");
    return (lastSep != std::string::npos) ? filepath.substr(lastSep + 1) : filepath;
}

std::string FileUtils::getDirectory(const std::string& filepath) {
    size_t lastSep = filepath.find_last_of("/\\");
    return (lastSep != std::string::npos) ? filepath.substr(0, lastSep) : "";
}

bool FileUtils::fileExists(const std::string& filepath) {
    DWORD dwAttrib = GetFileAttributesA(filepath.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

long long FileUtils::getFileSize(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return -1;
    }
    
    return static_cast<long long>(file.tellg());
}

bool FileUtils::createDirectory(const std::string& dirpath) {
    return CreateDirectoryA(dirpath.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

std::string FileUtils::readFileToString(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

bool FileUtils::writeStringToFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(content.c_str(), content.size());
    return file.good();
}

// === TimeUtils Implementation ===

std::string TimeUtils::getCurrentTimestamp(const std::string& format) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());
    return oss.str();
}

std::string TimeUtils::getCurrentISO8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}

long long TimeUtils::getCurrentMilliseconds() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::string TimeUtils::formatDuration(long long milliseconds) {
    long long seconds = milliseconds / 1000;
    long long minutes = seconds / 60;
    long long hours = minutes / 60;
    long long days = hours / 24;
    
    std::ostringstream oss;
    
    if (days > 0) {
        oss << days << "d ";
    }
    if (hours % 24 > 0) {
        oss << hours % 24 << "h ";
    }
    if (minutes % 60 > 0) {
        oss << minutes % 60 << "m ";
    }
    if (seconds % 60 > 0) {
        oss << seconds % 60 << "s";
    }
    
    std::string result = oss.str();
    return result.empty() ? "0s" : StringUtils::rtrim(result);
}

// === EncodingUtils Implementation ===

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string EncodingUtils::base64Encode(const std::vector<char>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(data.data());
    int in_len = static_cast<int>(data.size());
    
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];
        
        while((i++ < 3))
            ret += '=';
    }
    
    return ret;
}

std::string EncodingUtils::base64Encode(const std::string& str) {
    std::vector<char> data(str.begin(), str.end());
    return base64Encode(data);
}

std::vector<char> EncodingUtils::base64Decode(const std::string& encoded_string) {
    int in_len = static_cast<int>(encoded_string.size());
    int i = 0;
    int j = 0;
    int in = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<char> ret;
    
    while (in_len-- && ( encoded_string[in] != '=') && is_base64(encoded_string[in])) {
        char_array_4[i++] = encoded_string[in]; in++;
        if (i ==4) {
            for (i = 0; i <4; i++)
                char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; (i < 3); i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;
        
        for (j = 0; j <4; j++)
            char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        
        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }
    
    return ret;
}

std::string EncodingUtils::bytesToHex(const std::vector<char>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    
    for (unsigned char byte : data) {
        oss << std::setw(2) << static_cast<unsigned int>(byte);
    }
    
    return oss.str();
}

std::vector<char> EncodingUtils::hexToBytes(const std::string& hex) {
    std::vector<char> result;
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = static_cast<char>(std::strtol(byteString.c_str(), nullptr, 16));
        result.push_back(byte);
    }
    
    return result;
}

// === NetworkUtils Implementation ===

std::vector<char> NetworkUtils::createMessage(const std::string& content) {
    std::vector<char> result;
    
    if (content.length() <= 255) {
        // str8 format
        result.push_back(static_cast<char>(0xd9));
        result.push_back(static_cast<char>(content.length()));
        result.insert(result.end(), content.begin(), content.end());
    } else if (content.length() <= 65535) {
        // str16 format
        result.push_back(static_cast<char>(0xda));
        result.push_back(static_cast<char>((content.length() >> 8) & 0xFF));
        result.push_back(static_cast<char>(content.length() & 0xFF));
        result.insert(result.end(), content.begin(), content.end());
    } else {
        // str32 format
        result.push_back(static_cast<char>(0xdb));
        result.push_back(static_cast<char>((content.length() >> 24) & 0xFF));
        result.push_back(static_cast<char>((content.length() >> 16) & 0xFF));
        result.push_back(static_cast<char>((content.length() >> 8) & 0xFF));
        result.push_back(static_cast<char>(content.length() & 0xFF));
        result.insert(result.end(), content.begin(), content.end());
    }
    
    return result;
}

std::vector<char> NetworkUtils::createMessage(const std::map<std::string, std::string>& data) {
    std::vector<char> result;
    
    // Simple map format - not full msgpack implementation
    // This is a simplified version for the legacy compatibility
    std::ostringstream oss;
    for (const auto& pair : data) {
        oss << pair.first << "=" << pair.second << "\n";
    }
    
    return createMessage(oss.str());
}

bool NetworkUtils::sendAll(SOCKET socket, const char* data, size_t length) {
    size_t totalSent = 0;
    
    while (totalSent < length) {
        int sent = send(socket, data + totalSent, static_cast<int>(length - totalSent), 0);
        if (sent <= 0) {
            return false;
        }
        totalSent += sent;
    }
    
    return true;
}

bool NetworkUtils::recvAll(SOCKET socket, char* buffer, size_t length) {
    size_t totalReceived = 0;
    
    while (totalReceived < length) {
        int received = recv(socket, buffer + totalReceived, static_cast<int>(length - totalReceived), 0);
        if (received <= 0) {
            return false;
        }
        totalReceived += received;
    }
    
    return true;
}

bool NetworkUtils::sendMessage(SOCKET socket, const std::string& message) {
    auto messageData = createMessage(message);
    return sendAll(socket, messageData.data(), messageData.size());
}

bool NetworkUtils::sendMessage(SOCKET socket, const std::map<std::string, std::string>& data) {
    auto messageData = createMessage(data);
    return sendAll(socket, messageData.data(), messageData.size());
}

bool NetworkUtils::isValidIP(const std::string& ip) {
    std::regex ipRegex(
        R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)"
    );
    return std::regex_match(ip, ipRegex);
}

bool NetworkUtils::isValidPort(int port) {
    return port > 0 && port <= 65535;
}

// === SystemUtils Implementation ===

bool SystemUtils::createDirectory(const std::string& path) {
    if (path.empty()) return false;

    // For Windows, convert path separators and create recursively
    std::string windowsPath = path;
    std::replace(windowsPath.begin(), windowsPath.end(), '/', '\\');
    
    // Try to create directory
    if (CreateDirectoryA(windowsPath.c_str(), NULL) != 0) {
        return true; // Success
    }
    
    DWORD error = GetLastError();
    if (error == ERROR_ALREADY_EXISTS) {
        return true; // Already exists
    } else if (error == ERROR_PATH_NOT_FOUND) {
        // Parent doesn't exist, create it recursively
        size_t lastBackslash = windowsPath.find_last_of('\\');
        if (lastBackslash != std::string::npos) {
            std::string parentPath = windowsPath.substr(0, lastBackslash);
            if (createDirectory(parentPath)) {
                return CreateDirectoryA(windowsPath.c_str(), NULL) != 0;
            }
        }
    }
    return false;
}

std::string SystemUtils::getCurrentDirectory() {
    char buffer[MAX_PATH];
    if (_getcwd(buffer, MAX_PATH) != nullptr) {
        return std::string(buffer);
    }
    return "";
}

std::string SystemUtils::getExecutableDirectory() {
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (length > 0) {
        std::string path(buffer);
        return FileUtils::getDirectory(path);
    }
    return "";
}

std::string SystemUtils::getTempDirectory() {
    char buffer[MAX_PATH];
    DWORD length = GetTempPathA(MAX_PATH, buffer);
    if (length > 0) {
        return std::string(buffer);
    }
    return "C:\\temp\\";
}

void SystemUtils::sleep(int milliseconds) {
    Sleep(static_cast<DWORD>(milliseconds));
}

std::string SystemUtils::getEnvironmentVariable(const std::string& name, const std::string& defaultValue) {
    const char* value = getenv(name.c_str());
    return value ? std::string(value) : defaultValue;
}

std::string SystemUtils::getWindowsErrorMessage(DWORD errorCode) {
    if (errorCode == 0) {
        errorCode = GetLastError();
    }
    
    LPSTR messageBuffer = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);
    
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    
    return StringUtils::trim(message);
}

} // namespace Utils
} // namespace SkyRAT
