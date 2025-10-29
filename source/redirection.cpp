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

// Redirect stdout to a given filename (truncate or append depending on append flag).
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
int Shell::redirectStdoutToFile(const Redirection& redir) {
    if (redir.file.empty()) return -1;
    ensureParentDir(redir.file);

    int flags = O_WRONLY | O_CREAT | (redir.append ? O_APPEND : O_TRUNC);
    int fd = open(redir.file.c_str(), flags, 0644);
    if (fd < 0) { perror("open"); return -1; }

    int saved = dup(STDOUT_FILENO);
    if (saved < 0) { perror("dup"); close(fd); return -1; }

    if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); close(fd); close(saved); return -1; }

    close(fd);
    return saved;
}

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

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];

        // Append stdout: >> or 1>>
        if (token == ">>" || token == "1>>") {
            if (i + 1 < tokens.size()) {
                pr.stdoutFile = tokens[i + 1];
                pr.stdoutAppend = true;
                i++; // skip filename
            }
        }
        // Overwrite stdout: > or 1>
        else if (token == ">" || token == "1>") {
            if (i + 1 < tokens.size()) {
                pr.stdoutFile = tokens[i + 1];
                pr.stdoutAppend = false;
                i++;
            }
        }
        // Append stderr: 2>>
        else if (token == "2>>") {
            if (i + 1 < tokens.size()) {
                pr.stderrFile = tokens[i + 1];
                pr.stderrAppend = true;
                i++;
            }
        }
        // Overwrite stderr: 2>
        else if (token == "2>") {
            if (i + 1 < tokens.size()) {
                pr.stderrFile = tokens[i + 1];
                pr.stderrAppend = false;
                i++;
            }
        }
        // Otherwise, keep token as part of the command
        else {
            pr.tokens.push_back(token);
        }
    }

    return pr;
}


void Shell::setupRedirection(const std::string& stdoutFile, bool stdoutAppend,
                             const std::string& stderrFile, bool stderrAppend) {
    if (!stdoutFile.empty()) {
        ensureParentDir(stdoutFile);
        int flags = O_WRONLY | O_CREAT | (stdoutAppend ? O_APPEND : O_TRUNC);
        int fd = open(stdoutFile.c_str(), flags, 0644);
        if (fd < 0) { perror("open stdout redirect"); exit(1); }
        if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 stdout"); close(fd); exit(1); }
        close(fd);
    }

    if (!stderrFile.empty()) {
        ensureParentDir(stderrFile);
        int flags = O_WRONLY | O_CREAT | (stderrAppend ? O_APPEND : O_TRUNC);
        int fd = open(stderrFile.c_str(), flags, 0644);
        if (fd < 0) { perror("open stderr redirect"); exit(1); }
        if (dup2(fd, STDERR_FILENO) < 0) { perror("dup2 stderr"); close(fd); exit(1); }
        close(fd);
    }
}

// convenience wrapper
void Shell::setupRedirection(const ParseResult& pr) {
    setupRedirection(pr.stdoutFile, pr.stdoutAppend, pr.stderrFile, pr.stderrAppend);
}

void Shell::restoreStdout(int savedFd) {
    if (savedFd >= 0) {
        if (dup2(savedFd, STDOUT_FILENO) < 0) {
            perror("dup2 restore");
        }
        close(savedFd);
    }
}
