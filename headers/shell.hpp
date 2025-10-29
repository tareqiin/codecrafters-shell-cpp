#pragma once
#include <string>
#include <vector>


struct Redirection {
std::string file;
bool append = false;
};


struct ParseResult {
std::vector<std::string> tokens;
std::string stdoutFile;
bool stdoutAppend = false;
std::string stderrFile;
bool stderrAppend = false;
};


class Shell {
public:
void run();
void handleCommand(const std::string& input);


private:
std::vector<std::string> tokenize(const std::string& input);
ParseResult parseRedirection(const std::vector<std::string>& tokens);


// Primary implementation that accepts append flags
void setupRedirection(const std::string& stdoutFile, bool stdoutAppend,
const std::string& stderrFile, bool stderrAppend);
// Convenience overloads / wrappers
void setupRedirection(const ParseResult& pr);
void ensureParentDir(const std::string& path);


int redirectStdoutToFile(const std::string& file);
int redirectStdoutToFile(const Redirection& redir);
int redirectStderrToFile(const std::string& file);
void restoreStdout(int savedFd);


void builtinExit(const std::vector<std::string>& tokens);
void builtinEcho(const std::vector<std::string>& tokens);
void builtinType(const std::vector<std::string>& tokens);
void builtinPwd(const std::vector<std::string>& tokens);
void builtinCd(const std::vector<std::string>& tokens);
void executeExternal(const std::vector<std::string>& tokens);
};