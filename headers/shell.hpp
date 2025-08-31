#pragma once
#include <string>
#include <vector> 

class Shell {
public:
    void run(); // main loop
    void handleCommand(const std::string& input);
private:
    std::vector<std::string> tokenize(const std::string& input);

};