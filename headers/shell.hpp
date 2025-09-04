#pragma once
#include <string>
#include <vector>

struct Redirs {
    std::string stdoutFile;
    bool stdoutAppend = false;
    std::string stderrFile;
    bool stderrAppend = false;
};

struct ParseResult {
    std::vector<std::string> cleanTokens;
    Redirs redirs;
};

class Shell {
public:
    void run(); 
    void handleCommand(const std::string& input);

private:
    // Tokenize input string into arguments
    std::vector<std::string> tokenize(const std::string& input);

    // Parse redirection operators and return clean tokens + redirection info
    ParseResult parseRedirection(const std::vector<std::string>& tokens);

    // Redirect stdout/stderr to files
    void setupRedirection(const std::string& stdoutFile, const std::string& stderrFile);

    // Ensure parent directories exist before creating files
    void ensureParentDir(const std::string& path);

    // Save and redirect stdout/stderr
    int redirectStdoutToFile(const std::string& file);
    int redirectStderrToFile(const std::string& file);
    void restoreStdout(int savedFd); 

    // Builtin shell commands
    void builtinExit(const std::vector<std::string>& tokens);
    void builtinEcho(const std::vector<std::string>& tokens);
    void builtinType(const std::vector<std::string>& tokens);
    void builtinPwd(const std::vector<std::string>& tokens);
    void builtinCd(const std::vector<std::string>& tokens);

    // Execute external commands
    void executeExternal(const std::vector<std::string>& tokens);
};
