#include "../headers/shell.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>

void Shell::builtinExit(const std::vector<std::string>& tokens) {
    int code = 0;
    if (tokens.size() > 1) {
        code = std::stoi(tokens[1]);
    }
    exit(code);
}

void Shell::builtinEcho(const std::vector<std::string>& tokens) {
    ParseResult pr = parseRedirection(tokens);
    int savedStdout = redirectStdoutToFile(pr.stdoutFile);
    int savedStderr = redirectStderrToFile(pr.stderrFile);

    if (pr.tokens.empty()) {
        if (savedStdout >= 0) {
            if (dup2(savedStdout, STDOUT_FILENO) < 0) perror("dup2 restore");
            close(savedStdout);
        }
        if (savedStderr >= 0) {
            if (dup2(savedStderr, STDERR_FILENO) < 0) perror("dup2 restore");
            close(savedStderr);
        }
        return;
    }

    for (size_t i = 1; i < pr.tokens.size(); ++i) {
        if (i > 1) std::cout << " ";
        std::cout << pr.tokens[i];
    }
    std::cout << "\n";
    std::cout.flush();

    if (savedStdout >= 0) {
        if (dup2(savedStdout, STDOUT_FILENO) < 0) perror("dup2 restore");
        close(savedStdout);
    }
    if (savedStderr >= 0) {
        if (dup2(savedStderr, STDERR_FILENO) < 0) perror("dup2 restore");
        close(savedStderr);
    }
}

void Shell::builtinType(const std::vector<std::string>& tokens) {
    static const std::vector<std::string> builtins = {"echo", "exit", "type", "pwd", "cd"};
    if (tokens.size() < 2) return;
    std::string s = tokens[1];
    if (std::find(builtins.begin(), builtins.end(), s) != builtins.end()) {
        std::cout << s << " is a shell builtin\n";
        return;
    }
    char* pathEnv = getenv("PATH");
    bool found = false;
    if (pathEnv) {
        std::istringstream paths(pathEnv);
        std::string dir;
        while (std::getline(paths, dir, ':')) {
            std::string fullPath = dir + "/" + s;
            if (access(fullPath.c_str(), X_OK) == 0) {
                std::cout << s << " is " << fullPath << "\n";
                found = true;
                break;
            }
        }
    }
    if (!found) std::cout << s << ": not found\n";
}

void Shell::builtinPwd(const std::vector<std::string>& tokens) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << cwd << "\n";
    } else {
        perror("getcwd");
    }
}

void Shell::builtinCd(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        char* home = getenv("HOME");
        if (!home || chdir(home) != 0) {
            std::cout << "cd: HOME not set or cannot change directory\n";
        }
        return;
    }
    std::string path = tokens[1];
    if (path == "~") {
        char* home = getenv("HOME");
        if (!home || chdir(home) != 0) {
            std::cout << "cd: " << path << ": No such file or directory\n";
        }
    } else {
        if (chdir(path.c_str()) != 0) {
            std::cout << "cd: " << path << ": No such file or directory\n";
        }
    }
}
