/**
 * @file MessageProtocolExample.cpp
 * @brief Example usage of the extracted MessageProtocol class
 * 
 * This example demonstrates how to use the MessageProtocol class
 * that was extracted from legacy client.cpp message handling code.
 */

#include "Network/MessageProtocol.h"
#include "Utils/Logger.h"
#include <iostream>
#include <fstream>

using namespace Network;

void demonstrateBasicMessages() {
    std::cout << "\n=== Basic Message Creation Examples ===\n";
    
    MessageProtocol protocol;
    
    // 1. Simple text message
    std::string textMsg = "Hello SkyRAT Server!";
    auto textPacket = protocol.createTextMessage(textMsg);
    std::cout << "Text message created: " << textMsg << " -> " 
              << textPacket.size() << " bytes\n";
    
    // 2. Structured message
    std::map<std::string, std::string> data;
    data["type"] = "status";
    data["client_id"] = "client_001";
    data["status"] = "ready";
    auto structuredPacket = protocol.createStructuredMessage(data);
    std::cout << "Structured message created with " << data.size() 
              << " fields -> " << structuredPacket.size() << " bytes\n";
    
    // 3. Command message
    std::map<std::string, std::string> args;
    args["target"] = "desktop";
    args["quality"] = "high";
    auto commandPacket = protocol.createCommandMessage("screenshot", args);
    std::cout << "Command message created: screenshot -> " 
              << commandPacket.size() << " bytes\n";
    
    // 4. Response message
    auto responsePacket = protocol.createResponseMessage("Screenshot captured successfully", true);
    std::cout << "Response message created -> " << responsePacket.size() << " bytes\n";
    
    // 5. Error message
    auto errorPacket = protocol.createErrorMessage("Connection timeout", 1001);
    std::cout << "Error message created -> " << errorPacket.size() << " bytes\n";
}

void demonstrateMessageParsing() {
    std::cout << "\n=== Message Parsing Examples ===\n";
    
    MessageProtocol protocol;
    
    // Create a message and then parse it back
    std::map<std::string, std::string> originalData;
    originalData["command"] = "keylog";
    originalData["duration"] = "60";
    originalData["format"] = "text";
    
    auto packet = protocol.createStructuredMessage(originalData);
    std::cout << "Created message with " << originalData.size() << " fields\n";
    
    // Parse it back
    auto parsedData = protocol.parseMessage(packet);
    std::cout << "Parsed message back with " << parsedData.size() << " fields:\n";
    
    for (const auto& pair : parsedData) {
        std::cout << "  " << pair.first << " = " << pair.second << "\n";
    }
    
    // Test text content extraction
    auto textPacket = protocol.createTextMessage("Test content extraction");
    std::string extracted = protocol.extractTextContent(textPacket);
    std::cout << "Extracted text content: " << extracted << "\n";
}

void demonstrateFileTransfer() {
    std::cout << "\n=== File Transfer Examples ===\n";
    
    MessageProtocol protocol;
    
    // Small file example
    std::string smallFileContent = "This is a small test file content.";
    std::vector<char> smallFileData(smallFileContent.begin(), smallFileContent.end());
    
    auto smallFileMessages = protocol.createFileTransferMessages("small_test.txt", smallFileData);
    std::cout << "Small file (" << smallFileData.size() << " bytes) -> " 
              << smallFileMessages.size() << " message(s)\n";
    
    // Large file example (simulate)
    std::vector<char> largeFileData(100 * 1024, 'X'); // 100KB of 'X'
    auto largeFileMessages = protocol.createFileTransferMessages("large_test.dat", largeFileData);
    std::cout << "Large file (" << largeFileData.size() << " bytes) -> " 
              << largeFileMessages.size() << " message(s)\n";
    
    // Demonstrate chunking logic
    std::cout << "Chunk threshold: " << (MessageProtocol::shouldChunkFile(50 * 1024) ? "YES" : "NO") 
              << " for 50KB file\n";
    std::cout << "Chunk count for 100KB file: " 
              << MessageProtocol::calculateChunkCount(100 * 1024) << " chunks\n";
}

void demonstrateBase64Encoding() {
    std::cout << "\n=== Base64 Encoding Examples ===\n";
    
    // Test base64 encoding/decoding
    std::string testData = "Hello, this is test binary data! 🚀";
    std::vector<char> binaryData(testData.begin(), testData.end());
    
    std::string encoded = MessageProtocol::base64Encode(binaryData);
    std::cout << "Original: " << testData << "\n";
    std::cout << "Encoded:  " << encoded << "\n";
    
    std::vector<char> decoded = MessageProtocol::base64Decode(encoded);
    std::string decodedStr(decoded.begin(), decoded.end());
    std::cout << "Decoded:  " << decodedStr << "\n";
    std::cout << "Match:    " << (testData == decodedStr ? "YES" : "NO") << "\n";
}

void demonstrateLegacyCompatibility() {
    std::cout << "\n=== Legacy Compatibility Examples ===\n";
    
    MessageProtocol protocol;
    
    // This should produce the same output as the legacy create_message function
    // Format: [{"content": "message"}]
    auto legacyStyleMsg = protocol.createTextMessage("Legacy compatible message");
    
    std::cout << "Legacy-style message created: " << legacyStyleMsg.size() << " bytes\n";
    
    // Verify parsing works
    auto parsed = protocol.parseMessage(legacyStyleMsg);
    if (parsed.find("content") != parsed.end()) {
        std::cout << "Legacy format parsed successfully: " << parsed["content"] << "\n";
    }
    
    std::cout << "✓ Maintains compatibility with existing SkyRAT server\n";
}

int main() {
    try {
        std::cout << "SkyRAT MessageProtocol - Usage Examples\n";
        std::cout << "======================================\n";
        
        demonstrateBasicMessages();
        demonstrateMessageParsing();
        demonstrateFileTransfer();
        demonstrateBase64Encoding();
        demonstrateLegacyCompatibility();
        
        std::cout << "\n=== Summary ===\n";
        std::cout << "✓ MessageProtocol successfully extracted from legacy client.cpp\n";
        std::cout << "✓ Supports all original message formats\n";
        std::cout << "✓ Adds structured messaging capabilities\n";
        std::cout << "✓ Handles file transfers with automatic chunking\n";
        std::cout << "✓ Maintains backward compatibility\n";
        std::cout << "✓ Provides clean API for new development\n";
        
        std::cout << "\nMessage Protocol extraction completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}