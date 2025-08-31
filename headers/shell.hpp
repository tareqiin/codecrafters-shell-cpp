#pragma once
#include <string>
#include <vector> 

class Shell {
public:
    void run(); // main loop
private:
    vector<std::string> tokenize(const std::string& input); 
    void handleCommand(const std::string& input);
};