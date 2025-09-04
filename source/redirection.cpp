#include "../headers/shell.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <cerrno>
#include <string>
#include <sys/stat.h>
#include <cstdio>



int Shell::redirectStdoutToFile(const std::string &redirectFile) {
    if (redirectFile.empty()) return -1;
    ensureParentDir(redirectFile);

    int fd = open(redirectFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return -1; }

    int saved = dup(STDOUT_FILENO);
    if (saved < 0) { perror("dup"); close(fd); return -1; }

    if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); close(fd); close(saved); return -1; }

    close(fd);
    return saved;
}

int Shell::redirectStderrToFile(const std::string &redirectFile) {
    if (redirectFile.empty()) return -1;
    ensureParentDir(redirectFile);

    int fd = open(redirectFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return -1; }

    int saved = dup(STDERR_FILENO);
    if (saved < 0) { perror("dup"); close(fd); return -1; }

    if (dup2(fd, STDERR_FILENO) < 0) { perror("dup2"); close(fd); close(saved); return -1; }

    close(fd);
    return saved;
}


void Shell::ensureParentDir(const std::string& path) {
    size_t pos = 0;
    while ((pos = path.find('/', pos + 1)) != std::string::npos) {
        std::string dir = path.substr(0, pos);
        if (dir.empty()) continue;

        struct stat st{};
        if (stat(dir.c_str(), &st) != 0) {
            if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
                perror(("mkdir " + dir).c_str());
                return;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "%s exists and is not a directory\n", dir.c_str());
            return;
        }
    }
}

std::pair<std::vector<std::string>, std::pair<std::string, std::string>>
Shell::parseRedirection(const std::vector<std::string>& tokens) {
    std::string stdoutFile;
    std::string stderrFile;
    int redirectIndex = -1;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ">" || tokens[i] == "1>") {
            if (i + 1 < tokens.size()) {
                stdoutFile = tokens[i + 1];
                redirectIndex = static_cast<int>(i);
            } else {
                std::cerr << "Syntax error near unexpected token `newline'\n";
                return {{}, {"", ""}};
            }
            break;
        }
        if (tokens[i] == "2>") {
            if (i + 1 < tokens.size()) {
                stderrFile = tokens[i + 1];
                redirectIndex = static_cast<int>(i);
            } else {
                std::cerr << "Syntax error near unexpected token `newline'\n";
                return {{}, {"", ""}};
            }
            break;
        }
        if (tokens[i] == "1>>" || tokens[i] == ">>") {
            stdoutFile.append(tokens[i+1]); 
            redirectIndex = static_cast<int>(i); 
        } else {
            std::cerr << "Syntax error near unexpected token `newline` << we are hear>> \n"; 
            return {{}, {"", ""}}; 
        }
    }

    std::vector<std::string> cleanTokens;
    if (redirectIndex != -1) {
        cleanTokens.insert(cleanTokens.end(), tokens.begin(), tokens.begin() + redirectIndex);
    } else {
        cleanTokens = tokens;
    }

    return {cleanTokens, {stdoutFile, stderrFile}};
}
void Shell::setupRedirection(const std::string& redirectFile, const std::string& stderrFile) {
    if (!redirectFile.empty()) {
        ensureParentDir(redirectFile);

        int fd = open(redirectFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            exit(1);
        }

        close(fd);
    }

    if(!stderrFile.empty()) {
        ensureParentDir(stderrFile); 
        int fd = open(stderrFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); 
        if (fd < 0)  { perror("open"); exit(1); }
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2"); 
            close(fd); 
            exit(1); 
        }
        close(fd); 
    }
}