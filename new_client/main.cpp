/**
 * @file main.cpp
 * @brief Entry point for the SkyRAT client application
 * 
 * This file replaces the main() function from the legacy client.cpp,
 * providing a clean entry point that delegates to the Client class.
 */

#include "Core/Client.h"
#include <iostream>

int main() {
    try {
        SkyRAT::Core::Client client;
        return client.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}