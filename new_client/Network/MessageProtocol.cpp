#include "MessageProtocol.h"
#include "../Utils/Logger.h"

// Only include msgpack in the implementation file
#define MSGPACK_USE_BOOST 0
#define MSGPACK_NO_BOOST 1
#include "../Include/msgpack.hpp"

#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <sstream>

namespace Network {

// Implementation class to hide msgpack dependency
class MessageProtocol::Impl {
public:
    msgpack::v1::sbuffer buffer;
    
    void resetBuffer() {
        buffer.clear();
    }
};

MessageProtocol::MessageProtocol() : m_impl(std::make_unique<Impl>()) {
    // TODO: Add proper logging when logger instance is available
    // SkyRAT::Utils::Logger::info("MessageProtocol initialized");
}

MessageProtocol::~MessageProtocol() = default;

// === Message Creation Implementation ===

std::vector<char> MessageProtocol::createTextMessage(const std::string& content) const {
    // Utils::Logger::debug("Creating text message: " + content.substr(0, 50) + (content.length() > 50 ? "..." : ""));
    return createSimpleTextMessage(content);
}

std::vector<char> MessageProtocol::createStructuredMessage(const std::map<std::string, std::string>& data) const {
    // Utils::Logger::debug("Creating structured message with " + std::to_string(data.size()) + " fields");
    return createComplexMessage(data);
}

std::vector<std::vector<char>> MessageProtocol::createFileTransferMessages(
    const std::string& filename, 
    const std::vector<char>& fileData,
    size_t chunkSize
) const {
    // Utils::Logger::info("Creating file transfer for: " + filename + " (" + std::to_string(fileData.size()) + " bytes)");
    
    std::vector<std::vector<char>> messages;
    
    // For small files, send as single message
    if (!shouldChunkFile(fileData.size())) {
        std::string base64Data = base64Encode(fileData);
        
        std::map<std::string, std::string> msgData;
        msgData["type"] = TYPE_FILE_TRANSFER;
        msgData["filename"] = filename;
        msgData["filedata"] = base64Data;
        
        messages.push_back(createComplexMessage(msgData));
        // Utils::Logger::debug("Created single file transfer message");
        return messages;
    }
    
    // Large file - use chunked transfer
    size_t totalChunks = calculateChunkCount(fileData.size(), chunkSize);
    
    // Utils::Logger::info("Large file detected - using " + std::to_string(totalChunks) + " chunks of " + 
    //                    std::to_string(chunkSize) + " bytes each");
    
    // 1. Header chunk
    messages.push_back(createFileHeaderChunk(filename, fileData.size(), totalChunks));
    
    // 2. Data chunks
    for (size_t i = 0; i < totalChunks; ++i) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, fileData.size());
        
        std::vector<char> chunk(fileData.begin() + start, fileData.begin() + end);
        std::string base64Chunk = base64Encode(chunk);
        
        messages.push_back(createFileDataChunk(i, base64Chunk));
        
        // Log progress every 10 chunks
        if (i % 10 == 0 || i == totalChunks - 1) {
            // Utils::Logger::debug("Prepared chunk " + std::to_string(i + 1) + "/" + std::to_string(totalChunks));
        }
    }
    
    // 3. End marker
    messages.push_back(createFileEndChunk(filename));
    
    // Utils::Logger::info("File transfer messages prepared: " + std::to_string(messages.size()) + " total messages");
    return messages;
}

std::vector<char> MessageProtocol::createCommandMessage(
    const std::string& command, 
    const std::map<std::string, std::string>& args
) const {
    // Utils::Logger::debug("Creating command message: " + command);
    
    std::map<std::string, std::string> msgData;
    msgData["type"] = TYPE_COMMAND;
    msgData["command"] = command;
    
    // Add arguments
    for (const auto& arg : args) {
        msgData[arg.first] = arg.second;
    }
    
    return createComplexMessage(msgData);
}

std::vector<char> MessageProtocol::createResponseMessage(const std::string& response, bool success) const {
    // Utils::Logger::debug("Creating response message: " + std::string(success ? "SUCCESS" : "FAILURE"));
    
    std::map<std::string, std::string> msgData;
    msgData["type"] = TYPE_RESPONSE;
    msgData["response"] = response;
    msgData["success"] = success ? "true" : "false";
    
    return createComplexMessage(msgData);
}

std::vector<char> MessageProtocol::createErrorMessage(const std::string& error, int errorCode) const {
    // Utils::Logger::warn("Creating error message: " + error);
    
    std::map<std::string, std::string> msgData;
    msgData["type"] = TYPE_ERROR;
    msgData["error"] = error;
    if (errorCode != -1) {
        msgData["error_code"] = std::to_string(errorCode);
    }
    
    return createComplexMessage(msgData);
}

// === Message Parsing Implementation ===

std::map<std::string, std::string> MessageProtocol::parseMessage(const std::vector<char>& data) const {
    std::map<std::string, std::string> result;
    
    try {
        msgpack::v1::object_handle oh = msgpack::v1::unpack(data.data(), data.size());
        msgpack::v1::object obj = oh.get();
        
        // Expect array with one element
        if (obj.type != msgpack::v1::type::ARRAY || obj.via.array.size != 1) {
            // Utils::Logger::warn("Invalid message format: expected array with 1 element");
            return result;
        }
        
        msgpack::v1::object mapObj = obj.via.array.ptr[0];
        
        // Expect map
        if (mapObj.type != msgpack::v1::type::MAP) {
            // Utils::Logger::warn("Invalid message format: expected map in array");
            return result;
        }
        
        // Extract key-value pairs
        for (uint32_t i = 0; i < mapObj.via.map.size; ++i) {
            msgpack::v1::object key = mapObj.via.map.ptr[i].key;
            msgpack::v1::object val = mapObj.via.map.ptr[i].val;
            
            if (key.type == msgpack::v1::type::STR && val.type == msgpack::v1::type::STR) {
                std::string keyStr(key.via.str.ptr, key.via.str.size);
                std::string valStr(val.via.str.ptr, val.via.str.size);
                result[keyStr] = valStr;
            }
        }
        
        // Utils::Logger::debug("Parsed message with " + std::to_string(result.size()) + " fields");
        
    } catch (const std::exception& e) {
        // Utils::Logger::error("Failed to parse message: " + std::string(e.what()));
    }
    
    return result;
}

std::string MessageProtocol::extractTextContent(const std::vector<char>& data) const {
    auto parsed = parseMessage(data);
    auto it = parsed.find("content");
    
    if (it != parsed.end()) {
        // Utils::Logger::debug("Extracted text content: " + it->second.substr(0, 50) + 
        //                    (it->second.length() > 50 ? "..." : ""));
        return it->second;
    }
    
    // Utils::Logger::debug("No 'content' field found in message");
    return "";
}

bool MessageProtocol::isFileChunkMessage(const std::vector<char>& data) const {
    auto parsed = parseMessage(data);
    auto typeIt = parsed.find("type");
    
    bool isChunk = (typeIt != parsed.end() && typeIt->second == TYPE_FILE_CHUNK);
    // Utils::Logger::debug("Message is file chunk: " + std::string(isChunk ? "YES" : "NO"));
    
    return isChunk;
}

std::map<std::string, std::string> MessageProtocol::extractFileChunkInfo(const std::vector<char>& data) const {
    auto parsed = parseMessage(data);
    
    // Verify it's a file chunk message
    auto typeIt = parsed.find("type");
    if (typeIt == parsed.end() || typeIt->second != TYPE_FILE_CHUNK) {
        // Utils::Logger::warn("Attempted to extract chunk info from non-chunk message");
        return {};
    }
    
    // Utils::Logger::debug("Extracted file chunk info");
    return parsed;
}

// === Utility Functions Implementation ===

std::string MessageProtocol::base64Encode(const std::vector<char>& data) {
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

std::vector<char> MessageProtocol::base64Decode(const std::string& base64Data) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<char> result;
    
    int val = 0, valb = -8;
    for (char c : base64Data) {
        if (c == '=') break;
        
        size_t pos = chars.find(c);
        if (pos == std::string::npos) continue;
        
        val = (val << 6) + pos;
        valb += 6;
        if (valb >= 0) {
            result.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    
    return result;
}

bool MessageProtocol::shouldChunkFile(size_t fileSize) {
    return fileSize > LARGE_FILE_THRESHOLD;
}

size_t MessageProtocol::calculateChunkCount(size_t fileSize, size_t chunkSize) {
    return (fileSize + chunkSize - 1) / chunkSize;
}

// === Private Helper Functions ===

std::vector<char> MessageProtocol::createSimpleTextMessage(const std::string& content) const {
    // Create msgpack format: [{"content": "message"}]
    // This is the legacy format maintained for server compatibility
    
    std::vector<char> result;
    
    // Array with 1 element (fixarray format: 0x91)
    result.push_back(0x91);
    
    // Map with 1 key-value pair (fixmap format: 0x81)  
    result.push_back(0x81);
    
    // Key: "content" (7 chars, fixstr: 0xa7)
    result.push_back(0xa7);
    result.insert(result.end(), {'c','o','n','t','e','n','t'});
    
    // Value: message content
    auto encodedContent = encodeString(content);
    result.insert(result.end(), encodedContent.begin(), encodedContent.end());
    
    return result;
}

std::vector<char> MessageProtocol::createComplexMessage(const std::map<std::string, std::string>& data) const {
    try {
        m_impl->resetBuffer();
        msgpack::v1::packer<msgpack::v1::sbuffer> packer(&m_impl->buffer);
        
        // Main array with one element
        packer.pack_array(1);
        
        // Map with dynamic number of key-value pairs
        packer.pack_map(data.size());
        
        for (const auto& pair : data) {
            packer.pack(pair.first);
            packer.pack(pair.second);
        }
        
        std::vector<char> result(m_impl->buffer.data(), m_impl->buffer.data() + m_impl->buffer.size());
        return result;
        
    } catch (const std::exception& e) {
        // Utils::Logger::error("Failed to create complex message: " + std::string(e.what()));
        return {};
    }
}

std::vector<char> MessageProtocol::createFileHeaderChunk(
    const std::string& filename,
    size_t totalSize,
    size_t totalChunks
) const {
    std::map<std::string, std::string> headerData;
    headerData["type"] = TYPE_FILE_CHUNK;
    headerData["chunk_type"] = CHUNK_START;
    headerData["filename"] = filename;
    headerData["total_size"] = std::to_string(totalSize);
    headerData["total_chunks"] = std::to_string(totalChunks);
    
    return createComplexMessage(headerData);
}

std::vector<char> MessageProtocol::createFileDataChunk(size_t chunkNumber, const std::string& chunkData) const {
    std::map<std::string, std::string> chunkMsg;
    chunkMsg["type"] = TYPE_FILE_CHUNK;
    chunkMsg["chunk_type"] = CHUNK_DATA;
    chunkMsg["chunk_number"] = std::to_string(chunkNumber);
    chunkMsg["chunk_data"] = chunkData;
    
    return createComplexMessage(chunkMsg);
}

std::vector<char> MessageProtocol::createFileEndChunk(const std::string& filename) const {
    std::map<std::string, std::string> endData;
    endData["type"] = TYPE_FILE_CHUNK;
    endData["chunk_type"] = CHUNK_END;
    endData["filename"] = filename;
    
    return createComplexMessage(endData);
}

std::vector<char> MessageProtocol::encodeString(const std::string& str) const {
    std::vector<char> result;
    
    if (str.length() < 32) {
        // fixstr format (0xa0 + length)
        result.push_back(0xa0 + static_cast<char>(str.length()));
        result.insert(result.end(), str.begin(), str.end());
    } else if (str.length() < 256) {
        // str8 format  
        result.push_back(0xd9);
        result.push_back(static_cast<char>(str.length()));
        result.insert(result.end(), str.begin(), str.end());
    } else if (str.length() < 65536) {
        // str16 format
        result.push_back(0xda);
        result.push_back(static_cast<char>((str.length() >> 8) & 0xFF));
        result.push_back(static_cast<char>(str.length() & 0xFF));
        result.insert(result.end(), str.begin(), str.end());
    } else {
        // str32 format for very large strings
        result.push_back(0xdb);
        result.push_back(static_cast<char>((str.length() >> 24) & 0xFF));
        result.push_back(static_cast<char>((str.length() >> 16) & 0xFF));
        result.push_back(static_cast<char>((str.length() >> 8) & 0xFF));
        result.push_back(static_cast<char>(str.length() & 0xFF));
        result.insert(result.end(), str.begin(), str.end());
    }
    
    return result;
}

void MessageProtocol::addMapHeader(std::vector<char>& result, size_t mapSize) const {
    if (mapSize < 16) {
        // fixmap format (0x80 + size)
        result.push_back(0x80 + static_cast<char>(mapSize));
    } else if (mapSize < 65536) {
        // map16 format
        result.push_back(0xde);
        result.push_back(static_cast<char>((mapSize >> 8) & 0xFF));
        result.push_back(static_cast<char>(mapSize & 0xFF));
    } else {
        // map32 format
        result.push_back(0xdf);
        result.push_back(static_cast<char>((mapSize >> 24) & 0xFF));
        result.push_back(static_cast<char>((mapSize >> 16) & 0xFF));
        result.push_back(static_cast<char>((mapSize >> 8) & 0xFF));
        result.push_back(static_cast<char>(mapSize & 0xFF));
    }
}

void MessageProtocol::addArrayHeader(std::vector<char>& result, size_t arraySize) const {
    if (arraySize < 16) {
        // fixarray format (0x90 + size)
        result.push_back(0x90 + static_cast<char>(arraySize));
    } else if (arraySize < 65536) {
        // array16 format
        result.push_back(0xdc);
        result.push_back(static_cast<char>((arraySize >> 8) & 0xFF));
        result.push_back(static_cast<char>(arraySize & 0xFF));
    } else {
        // array32 format
        result.push_back(0xdd);
        result.push_back(static_cast<char>((arraySize >> 24) & 0xFF));
        result.push_back(static_cast<char>((arraySize >> 16) & 0xFF));
        result.push_back(static_cast<char>((arraySize >> 8) & 0xFF));
        result.push_back(static_cast<char>(arraySize & 0xFF));
    }
}

} // namespace Network
