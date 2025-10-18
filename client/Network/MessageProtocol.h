#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <winsock2.h>

// Use explicit msgpack v1 namespace to avoid ambiguity
#define MSGPACK_USE_BOOST 0
#define MSGPACK_NO_BOOST 1
#include "../Include/msgpack.hpp"

// Use v1 namespace explicitly to prevent ambiguity
namespace msgpack {
    using namespace v1;
}

namespace Network {

/**
 * @brief MessageProtocol handles all msgpack serialization/deserialization
 * 
 * This class extracts all message protocol logic from the legacy client.cpp
 * and provides a clean interface for creating and parsing msgpack messages
 * compatible with the SkyRAT Python server.
 * 
 * Supports:
 * - Simple text messages: [{"content": "message"}]
 * - Complex structured data: [{"key1": "value1", "key2": "value2"}]
 * - File transfer messages with base64 encoding
 * - Chunked file transfer for large files
 * - Binary data handling
 */
class MessageProtocol {
public:
    // Message type constants for structured data
    static constexpr const char* TYPE_FILE_TRANSFER = "file_transfer";
    static constexpr const char* TYPE_FILE_CHUNK = "file_chunk";
    static constexpr const char* TYPE_COMMAND = "command";
    static constexpr const char* TYPE_RESPONSE = "response";
    static constexpr const char* TYPE_ERROR = "error";
    
    // Chunk types for file transfer
    static constexpr const char* CHUNK_START = "start";
    static constexpr const char* CHUNK_DATA = "data";
    static constexpr const char* CHUNK_END = "end";
    
    // Configuration constants
    static constexpr size_t DEFAULT_CHUNK_SIZE = 32 * 1024;  // 32KB chunks
    static constexpr size_t LARGE_FILE_THRESHOLD = 64 * 1024; // 64KB threshold

public:
    MessageProtocol();
    ~MessageProtocol();

    // === Message Creation ===
    
    /**
     * @brief Create simple text message: [{"content": "text"}]
     * @param content The text content to send
     * @return Serialized msgpack data ready for socket transmission
     */
    std::vector<char> createTextMessage(const std::string& content) const;
    
    /**
     * @brief Create structured message from key-value pairs
     * @param data Map of key-value pairs to serialize
     * @return Serialized msgpack data ready for socket transmission
     */
    std::vector<char> createStructuredMessage(const std::map<std::string, std::string>& data) const;
    
    /**
     * @brief Create file transfer message with automatic chunking for large files
     * @param filename The name of the file being sent
     * @param fileData Binary file data
     * @param chunkSize Size of chunks for large files (default: 32KB)
     * @return Vector of serialized messages (single message for small files, multiple for chunked)
     */
    std::vector<std::vector<char>> createFileTransferMessages(
        const std::string& filename, 
        const std::vector<char>& fileData,
        size_t chunkSize = DEFAULT_CHUNK_SIZE
    ) const;
    
    /**
     * @brief Create command message
     * @param command The command to execute
     * @param args Additional command arguments (optional)
     * @return Serialized msgpack data
     */
    std::vector<char> createCommandMessage(
        const std::string& command, 
        const std::map<std::string, std::string>& args = {}
    ) const;
    
    /**
     * @brief Create response message
     * @param response The response content
     * @param success Whether the operation was successful
     * @return Serialized msgpack data
     */
    std::vector<char> createResponseMessage(const std::string& response, bool success = true) const;
    
    /**
     * @brief Create error message
     * @param error The error description
     * @param errorCode Optional error code
     * @return Serialized msgpack data
     */
    std::vector<char> createErrorMessage(const std::string& error, int errorCode = -1) const;

    // === Message Parsing ===
    
    /**
     * @brief Parse received msgpack data into structured format
     * @param data Raw msgpack data received from socket
     * @return Parsed key-value pairs, empty if parsing failed
     */
    std::map<std::string, std::string> parseMessage(const std::vector<char>& data) const;
    
    /**
     * @brief Extract text content from simple text message
     * @param data Raw msgpack data
     * @return Extracted text content, empty if not a text message
     */
    std::string extractTextContent(const std::vector<char>& data) const;
    
    /**
     * @brief Check if message is a file chunk
     * @param data Raw msgpack data
     * @return True if this is a file chunk message
     */
    bool isFileChunkMessage(const std::vector<char>& data) const;
    
    /**
     * @brief Extract file chunk information
     * @param data Raw msgpack data
     * @return Chunk information (type, number, data, etc.)
     */
    std::map<std::string, std::string> extractFileChunkInfo(const std::vector<char>& data) const;

    // === Utility Functions ===
    
    /**
     * @brief Encode binary data to base64 string
     * @param data Binary data to encode
     * @return Base64 encoded string
     */
    static std::string base64Encode(const std::vector<char>& data);
    
    /**
     * @brief Decode base64 string to binary data
     * @param base64Data Base64 encoded string
     * @return Decoded binary data
     */
    static std::vector<char> base64Decode(const std::string& base64Data);
    
    /**
     * @brief Check if file should be sent in chunks
     * @param fileSize Size of the file in bytes
     * @return True if file should be chunked
     */
    static bool shouldChunkFile(size_t fileSize);
    
    /**
     * @brief Calculate number of chunks needed for file
     * @param fileSize Size of the file in bytes
     * @param chunkSize Size of each chunk
     * @return Number of chunks needed
     */
    static size_t calculateChunkCount(size_t fileSize, size_t chunkSize = DEFAULT_CHUNK_SIZE);

private:
    // === Internal Message Creation Helpers ===
    
    /**
     * @brief Create msgpack message using manual encoding (faster for simple messages)
     * @param content Text content
     * @return Serialized msgpack data
     */
    std::vector<char> createSimpleTextMessage(const std::string& content) const;
    
    /**
     * @brief Create msgpack message using msgpack library (for complex data)
     * @param data Structured data
     * @return Serialized msgpack data
     */
    std::vector<char> createComplexMessage(const std::map<std::string, std::string>& data) const;
    
    /**
     * @brief Create file header chunk for chunked transfer
     * @param filename Name of the file
     * @param totalSize Total size of the file
     * @param totalChunks Total number of chunks
     * @return Serialized msgpack data for header
     */
    std::vector<char> createFileHeaderChunk(
        const std::string& filename,
        size_t totalSize,
        size_t totalChunks
    ) const;
    
    /**
     * @brief Create file data chunk
     * @param chunkNumber Current chunk number (0-based)
     * @param chunkData Base64 encoded chunk data
     * @return Serialized msgpack data for chunk
     */
    std::vector<char> createFileDataChunk(size_t chunkNumber, const std::string& chunkData) const;
    
    /**
     * @brief Create file end marker chunk
     * @param filename Name of the file
     * @return Serialized msgpack data for end marker
     */
    std::vector<char> createFileEndChunk(const std::string& filename) const;
    
    // === String Encoding Helpers ===
    
    /**
     * @brief Encode string in msgpack format (manual implementation)
     * @param str String to encode
     * @return Encoded string bytes
     */
    std::vector<char> encodeString(const std::string& str) const;
    
    /**
     * @brief Add msgpack map header
     * @param result Vector to append to
     * @param mapSize Number of key-value pairs in the map
     */
    void addMapHeader(std::vector<char>& result, size_t mapSize) const;
    
    /**
     * @brief Add msgpack array header
     * @param result Vector to append to
     * @param arraySize Number of elements in the array
     */
    void addArrayHeader(std::vector<char>& result, size_t arraySize) const;

private:
    // Pimpl idiom to hide msgpack dependency from header
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Network
