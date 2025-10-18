#include "../headers/mouse_module.h"
#include <iostream>
#include <windows.h>
#include <sstream> // Required for parsing move:x:y commands

const char* Mouse_Module::name() const {
    return "Mouse_Module";
}

void Mouse_Module::run() {
}

void Mouse_Module::moveMouse(int dx, int dy) {
    POINT p;
    GetCursorPos(&p);
    SetCursorPos(p.x + dx, p.y + dy);
}

void Mouse_Module::moveMouseTo(int x, int y) {
    SetCursorPos(x, y);
}

void Mouse_Module::processCommand(const std::string& cmd) {
    if (cmd == "up") {
        moveMouse(0, -10);
    } else if (cmd == "down") {
        moveMouse(0, 10);
    } else if (cmd == "left") {
        moveMouse(-10, 0);
    } else if (cmd == "right") {
        moveMouse(10, 0);
    } else if (cmd.rfind("move:", 0) == 0) {
        // Handle "move:x:y" command
        std::stringstream ss(cmd.substr(5)); // Create a stream from "x:y"
        std::string segment;
        int x, y;

        // Parse X
        if(std::getline(ss, segment, ':')) {
            x = std::stoi(segment);
        } else {
            return; // Invalid format
        }

        // Parse Y
        if(std::getline(ss, segment)) {
            y = std::stoi(segment);
        } else {
            return; // Invalid format
        }
        
        moveMouseTo(x, y);
    }
}