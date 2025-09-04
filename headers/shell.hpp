#pragma once
#include <string>
#include <vector>

class Shell {
public:
    void run();
    void handleCommand(const std::string& input);

private:
    std::vector<std::string> tokenize(const std::string& input);

    std::pair<std::vector<std::string>, std::pair<std::string, std::string>>
    parseRedirection(const std::vector<std::string>& tokens);

    void setupRedirection(const std::string& stdoutFile, const std::string& stderrFile);
    void ensureParentDir(const std::string& path);

    int redirectStdoutToFile(const std::string &redirectFile);
    int redirectStderrToFile(const std::string &redirectFile); // new
};
