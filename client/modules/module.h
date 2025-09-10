#pragma once
#include <string>
#include <vector>
#include <winsock2.h>
#define MSGPACK_USE_BOOST 0
#define MSGPACK_NO_BOOST 1
#include <msgpack.hpp>

class Module {
public:
    virtual ~Module() = default;
    virtual const char* name() const = 0;
    virtual void run() = 0;

    bool sendFileViaMsgPack(SOCKET sock, const char* fileName);
protected:
    std::vector<char> create_message(const std::string& content);
    std::string base64_encode(const std::vector<char>& data);
    bool sendFileInChunks(SOCKET sock, const char* filename, const std::vector<char>& filedata);
    bool send_message(SOCKET sock, const std::string& msg);
};
