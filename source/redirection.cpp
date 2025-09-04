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

// Redirect stdout to a given filename (truncate).
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

// Redirect stdout using a Redirection descriptor (supports append).

int Shell::redirectStderrToFile(const std::string &file) {
    if (file.empty()) return -1;
    ensureParentDir(file);
    int fd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open stderr"); return -1; }
    int saved = dup(STDERR_FILENO);
    if (saved < 0) { perror("dup stderr"); close(fd); return -1; }
    if (dup2(fd, STDERR_FILENO) < 0) { perror("dup2 stderr"); close(fd); close(saved); return -1; }
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

static bool is_all_digits(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(),
        [](unsigned char ch){ return std::isdigit(ch); });
}

ParseResult Shell::parseRedirection(const std::vector<std::string>& tokens) {
    ParseResult pr;
    pr.tokens = tokens; 

    for (size_t i = 0; i < pr.tokens.size();) {
        const std::string &tok = pr.tokens[i];

        bool handled = false;
        if (tok == ">" || tok == "1>") {
            if (i + 1 < pr.tokens.size()) {
                pr.stdoutFile = pr.tokens[i + 1];
                pr.stdoutAppend = false;
                pr.tokens.erase(pr.tokens.begin() + i, pr.tokens.begin() + i + 2);
                handled = true;
            } else {
                std::cerr << "Syntax error near unexpected token `newline'\n";
                return ParseResult{}; 
            }
        } else if (tok == ">>" || tok == "1>>") {
            if (i + 1 < pr.tokens.size()) {
                pr.stdoutFile = pr.tokens[i + 1];
                pr.stdoutAppend = true;
                pr.tokens.erase(pr.tokens.begin() + i, pr.tokens.begin() + i + 2);
                handled = true;
            } else {
                std::cerr << "Syntax error near unexpected token `newline'\n";
                return ParseResult{};
            }
        } else if (tok == "2>" || tok == "2>>") {
            if (i + 1 < pr.tokens.size()) {
                pr.stderrFile = pr.tokens[i + 1];
                pr.stderrAppend = (tok == "2>>");
                pr.tokens.erase(pr.tokens.begin() + i, pr.tokens.begin() + i + 2);
                handled = true;
            } else {
                std::cerr << "Syntax error near unexpected token `newline'\n";
                return ParseResult{};
            }
        } else if (tok.size() > 1 && is_all_digits(tok.substr(0, tok.size()-1)) && (tok.back() == '>' || (tok.size() > 1 && tok.substr(tok.size()-2) == ">>"))) {

        }

        if (!handled) {
            ++i;
        }
    }

    return pr;
}

void Shell::setupRedirection(const std::string& stdoutFile, const std::string& stderrFile) {
    if (!stdoutFile.empty()) {
        ensureParentDir(stdoutFile);
        int flags = O_WRONLY | O_CREAT | O_TRUNC;
        int fd = open(stdoutFile.c_str(), flags, 0644);
        if (fd < 0) { perror("open stdout redirect"); exit(1); }
        if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 stdout"); close(fd); exit(1); }
        close(fd);
    }

    if (!stderrFile.empty()) {
        ensureParentDir(stderrFile);
        int flags = O_WRONLY | O_CREAT | O_TRUNC;
        int fd = open(stderrFile.c_str(), flags, 0644);
        if (fd < 0) { perror("open stderr redirect"); exit(1); }
        if (dup2(fd, STDERR_FILENO) < 0) { perror("dup2 stderr"); close(fd); exit(1); }
        close(fd);
    }
}

void Shell::restoreStdout(int savedFd) {
    if (savedFd >= 0) {
        if (dup2(savedFd, STDOUT_FILENO) < 0) {
            perror("dup2 restore");
        }
        close(savedFd);
    }
}
