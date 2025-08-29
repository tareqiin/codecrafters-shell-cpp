#pragma once
#include <string>

class Shell {
public:
    void run(); // main loop
private:
    void handleCommand(const std::string& input);
};