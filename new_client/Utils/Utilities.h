#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <functional>
#include <winsock2.h>
#include <windows.h>

namespace SkyRAT {
namespace Utils {

    /**
     * @brief String manipulation utilities
     */
    class StringUtils {
    public:
        /**
         * @brief Trim whitespace from both ends of string
         * @param str String to trim
         * @return Trimmed string
         */
        static std::string trim(const std::string& str);

        /**
         * @brief Trim whitespace from left end of string
         * @param str String to trim
         * @return Left-trimmed string
         */
        static std::string ltrim(const std::string& str);

        /**
         * @brief Trim whitespace from right end of string
         * @param str String to trim
         * @return Right-trimmed string
         */
        static std::string rtrim(const std::string& str);

        /**
         * @brief Split string by delimiter
         * @param str String to split
         * @param delimiter Delimiter character
         * @return Vector of split strings
         */
        static std::vector<std::string> split(const std::string& str, char delimiter);

        /**
         * @brief Split string by delimiter string
         * @param str String to split
         * @param delimiter Delimiter string
         * @return Vector of split strings
         */
        static std::vector<std::string> split(const std::string& str, const std::string& delimiter);

        /**
         * @brief Join strings with delimiter
         * @param strings Vector of strings to join
         * @param delimiter Delimiter to use
         * @return Joined string
         */
        static std::string join(const std::vector<std::string>& strings, const std::string& delimiter);

        /**
         * @brief Convert string to lowercase
         * @param str String to convert
         * @return Lowercase string
         */
        static std::string toLower(const std::string& str);

        /**
         * @brief Convert string to uppercase
         * @param str String to convert
         * @return Uppercase string
         */
        static std::string toUpper(const std::string& str);

        /**
         * @brief Check if string starts with prefix
         * @param str String to check
         * @param prefix Prefix to look for
         * @return true if string starts with prefix
         */
        static bool startsWith(const std::string& str, const std::string& prefix);

        /**
         * @brief Check if string ends with suffix
         * @param str String to check
         * @param suffix Suffix to look for
         * @return true if string ends with suffix
         */
        static bool endsWith(const std::string& str, const std::string& suffix);

        /**
         * @brief Replace all occurrences of search string with replacement
         * @param str String to modify
         * @param search String to search for
         * @param replace String to replace with
         * @return Modified string
         */
        static std::string replace(const std::string& str, const std::string& search, const std::string& replace);

        /**
         * @brief Convert std::string to std::wstring (Windows only)
         * @param str String to convert
         * @return Wide string
         */
        static std::wstring stringToWstring(const std::string& str);

        /**
         * @brief Convert std::wstring to std::string (Windows only)
         * @param wstr Wide string to convert
         * @return Regular string
         */
        static std::string wstringToString(const std::wstring& wstr);
    };

    /**
     * @brief File and path utilities
     */
    class FileUtils {
    public:
        /**
         * @brief Extract directory path from a file path
         * @param filepath Full file path
         * @return Directory portion of the path
         */
        static std::string extractDirectory(const std::string& filepath);
        
        /**
         * @brief Check if a directory exists
         * @param path Directory path to check
         * @return true if directory exists, false otherwise
         */
        static bool directoryExists(const std::string& path);
        
        /**
         * @brief Sanitize filename for safe filesystem usage
         * @param filename Filename to sanitize
         * @return Sanitized filename
         */
        static std::string sanitizeFileName(const std::string& filename);

        /**
         * @brief Get file extension from path
         * @param filepath File path
         * @return File extension (with dot)
         */
        static std::string getFileExtension(const std::string& filepath);

        /**
         * @brief Get filename from path (without directory)
         * @param filepath File path
         * @return Filename only
         */
        static std::string getFileName(const std::string& filepath);

        /**
         * @brief Get directory from path (without filename)
         * @param filepath File path
         * @return Directory path
         */
        static std::string getDirectory(const std::string& filepath);

        /**
         * @brief Check if file exists
         * @param filepath File path to check
         * @return true if file exists
         */
        static bool fileExists(const std::string& filepath);

        /**
         * @brief Get file size in bytes
         * @param filepath File path
         * @return File size or -1 if error
         */
        static long long getFileSize(const std::string& filepath);

        /**
         * @brief Create directory recursively
         * @param dirpath Directory path to create
         * @return true if successful
         */
        static bool createDirectory(const std::string& dirpath);

        /**
         * @brief Read entire file into string
         * @param filepath File path to read
         * @return File contents or empty string if error
         */
        static std::string readFileToString(const std::string& filepath);

        /**
         * @brief Write string to file
         * @param filepath File path to write
         * @param content Content to write
         * @return true if successful
         */
        static bool writeStringToFile(const std::string& filepath, const std::string& content);
    };

    /**
     * @brief Time and date utilities
     */
    class TimeUtils {
    public:
        /**
         * @brief Get current timestamp as string
         * @param format Format string (default: "%Y-%m-%d %H:%M:%S")
         * @return Formatted timestamp
         */
        static std::string getCurrentTimestamp(const std::string& format = "%Y-%m-%d %H:%M:%S");

        /**
         * @brief Get current timestamp in ISO 8601 format
         * @return ISO 8601 timestamp
         */
        static std::string getCurrentISO8601();

        /**
         * @brief Get milliseconds since epoch
         * @return Milliseconds since epoch
         */
        static long long getCurrentMilliseconds();

        /**
         * @brief Format duration in human-readable format
         * @param milliseconds Duration in milliseconds
         * @return Formatted duration (e.g., "1h 23m 45s")
         */
        static std::string formatDuration(long long milliseconds);
    };

    /**
     * @brief Encoding and conversion utilities
     */
    class EncodingUtils {
    public:
        /**
         * @brief Encode data to Base64
         * @param data Data to encode
         * @return Base64 encoded string
         */
        static std::string base64Encode(const std::vector<char>& data);

        /**
         * @brief Encode string to Base64
         * @param str String to encode
         * @return Base64 encoded string
         */
        static std::string base64Encode(const std::string& str);

        /**
         * @brief Decode Base64 string
         * @param encoded Base64 encoded string
         * @return Decoded data
         */
        static std::vector<char> base64Decode(const std::string& encoded);

        /**
         * @brief Convert bytes to hex string
         * @param data Data to convert
         * @return Hex string
         */
        static std::string bytesToHex(const std::vector<char>& data);

        /**
         * @brief Convert hex string to bytes
         * @param hex Hex string
         * @return Decoded bytes
         */
        static std::vector<char> hexToBytes(const std::string& hex);
    };

    /**
     * @brief Network utilities
     */
    class NetworkUtils {
    public:
        /**
         * @brief Create msgpack message from string content
         * @param content String content
         * @return Msgpack formatted data
         */
        static std::vector<char> createMessage(const std::string& content);

        /**
         * @brief Create msgpack message from key-value pairs
         * @param data Key-value map
         * @return Msgpack formatted data
         */
        static std::vector<char> createMessage(const std::map<std::string, std::string>& data);

        /**
         * @brief Send all data through socket
         * @param socket Socket to send through
         * @param data Data to send
         * @param length Length of data
         * @return true if all data sent successfully
         */
        static bool sendAll(SOCKET socket, const char* data, size_t length);

        /**
         * @brief Receive all data from socket
         * @param socket Socket to receive from
         * @param buffer Buffer to store data
         * @param length Length of data to receive
         * @return true if all data received successfully
         */
        static bool recvAll(SOCKET socket, char* buffer, size_t length);

        /**
         * @brief Send string message through socket
         * @param socket Socket to send through
         * @param message Message to send
         * @return true if sent successfully
         */
        static bool sendMessage(SOCKET socket, const std::string& message);

        /**
         * @brief Send structured data through socket
         * @param socket Socket to send through
         * @param data Key-value data to send
         * @return true if sent successfully
         */
        static bool sendMessage(SOCKET socket, const std::map<std::string, std::string>& data);

        /**
         * @brief Validate IP address format
         * @param ip IP address string to validate
         * @return true if valid IP address
         */
        static bool isValidIP(const std::string& ip);

        /**
         * @brief Validate port number
         * @param port Port number to validate
         * @return true if valid port (1-65535)
         */
        static bool isValidPort(int port);
    };

    /**
     * @brief System utilities
     */
    class SystemUtils {
    public:
        /**
         * @brief Create directory (including parent directories)
         * @param path Directory path to create
         * @return true if created successfully or already exists
         */
        static bool createDirectory(const std::string& path);
        
        /**
         * @brief Get current working directory
         * @return Current working directory path
         */
        static std::string getCurrentDirectory();

        /**
         * @brief Get executable directory
         * @return Directory containing current executable
         */
        static std::string getExecutableDirectory();

        /**
         * @brief Get system temporary directory
         * @return System temporary directory path
         */
        static std::string getTempDirectory();

        /**
         * @brief Sleep for specified milliseconds
         * @param milliseconds Time to sleep
         */
        static void sleep(int milliseconds);

        /**
         * @brief Get environment variable
         * @param name Variable name
         * @param defaultValue Default value if not found
         * @return Environment variable value
         */
        static std::string getEnvironmentVariable(const std::string& name, const std::string& defaultValue = "");

        /**
         * @brief Get Windows system error message
         * @param errorCode Error code (default: GetLastError())
         * @return Error message string
         */
        static std::string getWindowsErrorMessage(DWORD errorCode = 0);
    };

} // namespace Utils
} // namespace SkyRAT
