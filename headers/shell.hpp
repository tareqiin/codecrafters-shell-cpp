#pragma once
#include <string>
#include <vector>
#include <utility>


struct ParseResult {
    std::vector<std::string> cleanTokens;
    std::string stdoutFile;
    bool stdoutAppend = false;
    std::string stderrFile;
    bool stderrAppend = false;
};
struct Redirection {
    std::string file;
    bool append = false;
};

struct Redirs {
    Redirection stdoutRedir;
    Redirection stderrRedir;
};


class Shell {
public:
    void run(); 
    void handleCommand(const std::string& input);

private:
    std::vector<std::string> tokenize(const std::string& input);

    std::pair<std::vector<std::string>, Redirs>
    Shell::parseRedirection(const std::vector<std::string>& tokens); 

    void setupRedirection(const std::string& stdoutFile, const std::string& stderrFile);
    void ensureParentDir(const std::string& path);

    int redirectStdoutToFile(const Redirection& redir);
    int redirectStderrToFile(const Redirection& redir);


    void restoreStdout(int savedFd); 

    // builtins
    void builtinExit(const std::vector<std::string>& tokens);
    void builtinEcho(const std::vector<std::string>& tokens);
    void builtinType(const std::vector<std::string>& tokens);
    void builtinPwd(const std::vector<std::string>& tokens);
    void builtinCd(const std::vector<std::string>& tokens);
    void executeExternal(const std::vector<std::string>& tokens);
};
