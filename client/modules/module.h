#pragma once

class Module {
public:
    virtual ~Module() = default;
    virtual const char* name() const = 0;
    virtual void run() = 0;
};
