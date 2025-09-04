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

    // builtins 

    void builtinExit(const std::vector<std::string>& tokens);
    void builtinEcho(const std::vector<std::string>& tokens);
    void builtinType(const std::vector<std::string>& tokens);
    void builtinPwd(const std::vector<std::string>& tokens);
    void builtinCd(const std::vector<std::string>& tokens);
    void executeExternal(const std::vector<std::string>& tokens);

};
