#include "../headers/module.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <winsock2.h>
#include <msgpack.hpp>
#include <thread>
#include <chrono>
#include <map>

// Function to send a file over socket using msgpack protocol
bool Module::sendFileViaMsgPack(SOCKET sock, const char* fileName){
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
        std::map<std::string, std::string> msg_data;
        msg_data["type"] = "file_transfer";
        msg_data["filename"] = fileName;
        msg_data["filedata"] = base64_data;
        return send_message(sock, msg_data);
}

// Function to send file in chunks for large files
bool Module::sendFileInChunks(SOCKET sock, const char* filename, const std::vector<char>& filedata){
    const size_t chunk_size = 32 * 1024; // 32KB chunks
    size_t total_chunks = (filedata.size() + chunk_size - 1) / chunk_size;
    
    std::cout << "[Info] Sending file in " << total_chunks << " chunks of " << chunk_size << " bytes each" << std::endl;
    
    // Send file header
    std::map<std::string, std::string> header_data;
    header_data["type"] = "file_chunk";
    header_data["chunk_type"] = "start";
    header_data["filename"] = filename;
    header_data["total_size"] = std::to_string(filedata.size());
    header_data["total_chunks"] = std::to_string(total_chunks);
    if (!send_message(sock, header_data)) return false;
    
    // Send chunks
    for (size_t i = 0; i < total_chunks; ++i) {
        size_t start = i * chunk_size;
        size_t end = std::min(start + chunk_size, filedata.size());
        
        std::vector<char> chunk(filedata.begin() + start, filedata.begin() + end);
        std::string base64_chunk = base64_encode(chunk);
        
        std::map<std::string, std::string> chunk_data;
        chunk_data["type"] = "file_chunk";
        chunk_data["chunk_type"] = "data";
        chunk_data["chunk_number"] = std::to_string(i);
        chunk_data["chunk_data"] = base64_chunk;

        if (!send_message(sock, chunk_data)) {
            std::cerr << "[Error] Failed to send chunk " << i << "/" << total_chunks << std::endl;
            return false;
        }
        
        // Show progress every 10 chunks
        if (i % 10 == 0 || i == total_chunks - 1) {
            std::cout << "[Progress] Sent chunk " << (i + 1) << "/" << total_chunks << std::endl;
        }
        
        // Small delay between chunks to avoid overwhelming the connection
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    std::cout << "[Success] File transfer completed - " << total_chunks << " chunks sent" << std::endl;
    
    // Send file end marker
    std::map<std::string, std::string> end_data;
    end_data["type"] = "file_chunk";
    end_data["chunk_type"] = "end";
    end_data["filename"] = filename;
    return send_message(sock, end_data);
}

// Simple base64 encoding function
std::string Module::base64_encode(const std::vector<char>& data){
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

// Send message over socket using server's expected format
bool Module::send_message(SOCKET sock, const std::string& msg){
    std::vector<char> formatted_msg = create_message(msg);
    return send(sock, formatted_msg.data(), static_cast<int>(formatted_msg.size()), 0) != SOCKET_ERROR;
}

bool Module::send_message(SOCKET sock, const std::map<std::string, std::string>& data) {
    std::vector<char> formatted_msg = create_message(data);
    return send(sock, formatted_msg.data(), static_cast<int>(formatted_msg.size()), 0) != SOCKET_ERROR;
}

// Simple msgpack message structure for server compatibility
std::vector<char> Module::create_message(const std::string& content) {
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

std::vector<char> Module::create_message(const std::map<std::string, std::string>& data) {
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> packer(&sbuf);

    // Main array with one element
    packer.pack_array(1);

    // Map with dynamic number of key-value pairs
    packer.pack_map(data.size());

    for (const auto& pair : data) {
        packer.pack(pair.first);
        packer.pack(pair.second);
    }

    std::vector<char> result(sbuf.data(), sbuf.data() + sbuf.size());
    return result;
}