#pragma once
#include "module.h"

void TakeScreenshot(const char* filename);

class SS_Module : public Module {
public:
    const char* name() const override;
    void run() override;
};
