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
int Shell::redirectStdoutToFile(const Redirection& redir) {
    if (redir.file.empty()) return -1;
    ensureParentDir(redir.file);

    int flags = O_WRONLY | O_CREAT | (redir.append ? O_APPEND : O_TRUNC);
    int fd = open(redir.file.c_str(), flags, 0644);
    if (fd < 0) { perror("open"); return -1; }

    int saved = dup(STDOUT_FILENO);
    if (saved < 0) { perror("dup"); close(fd); return -1; }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        perror("dup2");
        close(fd);
        close(saved);
        return -1;
    }

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
std::pair<std::vector<std::string>, std::pair<std::string, std::string>>
ParseResult Shell::parseRedirection(const std::vector<std::string>& tokens) {
    ParseResult res;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string &t = tokens[i];

        // Combined tokens first
        if (t == ">" || t == "1>") {
            if (i + 1 >= tokens.size()) { std::cerr << "Syntax error near unexpected token `newline'\n"; return res; }
            res.stdoutFile = tokens[i+1];
            res.stdoutAppend = false;
            ++i;
            continue;
        }
        if (t == ">>" || t == "1>>") {
            if (i + 1 >= tokens.size()) { std::cerr << "Syntax error near unexpected token `newline'\n"; return res; }
            res.stdoutFile = tokens[i+1];
            res.stdoutAppend = true;
            ++i;
            continue;
        }
        if (t == "2>") {
            if (i + 1 >= tokens.size()) { std::cerr << "Syntax error near unexpected token `newline'\n"; return res; }
            res.stderrFile = tokens[i+1];
            res.stderrAppend = false;
            ++i;
            continue;
        }
        if (t == "2>>") {
            if (i + 1 >= tokens.size()) { std::cerr << "Syntax error near unexpected token `newline'\n"; return res; }
            res.stderrFile = tokens[i+1];
            res.stderrAppend = true;
            ++i;
            continue;
        }

        // Split forms: ["2", ">", "file"] or ["2", ">>", "file"] or ["1", ">>", "file"]
        if (is_all_digits(t) && i + 2 < tokens.size() &&
            (tokens[i+1] == ">" || tokens[i+1] == ">>")) {
            const std::string &op = tokens[i+1];
            const std::string &fname = tokens[i+2];
            if (t == "2") {
                res.stderrFile = fname;
                res.stderrAppend = (op == ">>");
            } else {
                // treat other fd numbers as stdout (1) redirection
                res.stdoutFile = fname;
                res.stdoutAppend = (op == ">>");
            }
            i += 2; // skip the operator and filename
            continue;
        }

        // Otherwise this token is a normal command/arg
        res.cleanTokens.push_back(t);
    }

    return res;
}

void Shell::setupRedirection(const ParseResult &pr) {
    // stdout
    if (!pr.stdoutFile.empty()) {
        ensureParentDir(pr.stdoutFile);
        int flags = O_WRONLY | O_CREAT | (pr.stdoutAppend ? O_APPEND : O_TRUNC);
        int fd = open(pr.stdoutFile.c_str(), flags, 0644);
        if (fd < 0) { perror("open stdout redirect"); exit(1); }
        if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 stdout"); close(fd); exit(1); }
        close(fd);
    }

    // stderr
    if (!pr.stderrFile.empty()) {
        ensureParentDir(pr.stderrFile);
        int flags = O_WRONLY | O_CREAT | (pr.stderrAppend ? O_APPEND : O_TRUNC);
        int fd = open(pr.stderrFile.c_str(), flags, 0644);
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

