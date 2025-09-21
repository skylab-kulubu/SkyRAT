#pragma once

#include <string>

namespace mouse_control {

void moveMouse(const std::string& cmd);
void startMouseClient(const char* serverIp, int port);

} // namespace mouse_control
