#include "ss_module.h"
#include <iostream>

int main() {
    SS_Module ssModule;
    std::cout << "Module Name: " << ssModule.name() << std::endl;
    ssModule.run();
    return 0;
}