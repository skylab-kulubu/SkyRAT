#pragma once
#include "module.h"
#include <string>

class Mouse_Module : public Module {
public:
    const char* name() const override;
    void run() override;
    void processCommand(const std::string& cmd);

private:
    void moveMouse(int dx, int dy);
    void moveMouseTo(int x, int y);
};