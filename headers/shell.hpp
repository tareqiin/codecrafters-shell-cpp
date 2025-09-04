#pragma once
#include <string>
#include <vector> 

class Shell {
public:
    void run(); // main loop
    void handleCommand(const std::string& input);
private:
    std::vector<std::string> tokenize(const std::string& input);
    std::pair<std::vector<std::string>, std::pair<std::string, std::string>>
    parseRedirection(const std::vector<std::string>& tokens); 
    void setupRedirection(const std::string& redirectFile); 
    void ensureParentDir(const std::string& path); 
    int redirectStdoutToFile(const std::string &redirectFile); 
    int redirectStdoutToFile(const std::string &redirectFile); 

};