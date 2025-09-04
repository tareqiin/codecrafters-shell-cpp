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
Shell::parseRedirection(const std::vector<std::string>& tokens) {
   Redirs redirs;
int redirectIndex = -1;

for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i] == ">" || tokens[i] == "1>") {
        if (i + 1 < tokens.size()) {
            redirs.stdoutRedir.file = tokens[i + 1];
            redirs.stdoutRedir.append = false;
            redirectIndex = static_cast<int>(i);
        } else {
            std::cerr << "Syntax error near unexpected token `newline'\n";
            return {{}, {}};
        }
        break;
    }
    if (tokens[i] == ">>" || tokens[i] == "1>>") {
        if (i + 1 < tokens.size()) {
            redirs.stdoutRedir.file = tokens[i + 1];
            redirs.stdoutRedir.append = true;
            redirectIndex = static_cast<int>(i);
        } else {
            std::cerr << "Syntax error near unexpected token `newline'\n";
            return {{}, {}};
        }
        break;
    }
    if (tokens[i] == "2>" || tokens[i] == "2>>") {
        if (i + 1 < tokens.size()) {
            redirs.stderrRedir.file = tokens[i + 1];
            redirs.stderrRedir.append = (tokens[i] == "2>>");
            redirectIndex = static_cast<int>(i);
        } else {
            std::cerr << "Syntax error near unexpected token `newline'\n";
            return {{}, {}};
        }
        break;
    }
}

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

