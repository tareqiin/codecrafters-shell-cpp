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
ParseResult Shell::parseRedirection(const std::vector<std::string> &tokens)
{
    ParseResult result;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const std::string &token = tokens[i];
        bool consumed = false;

        // Check if token includes a redirection pattern like "1>", "1>>", "2>", etc.
        size_t pos = 0;
        bool append = false;
        FileDescriptor fd = FileDescriptor::STDOUT;

        // Detect file descriptor (0, 1, or 2)
        if (token.size() >= 2 && std::isdigit(token[0]) && token[1] == '>')
        {
            fd = static_cast<FileDescriptor>(token[0] - '0');
            pos = 1;
        }
        else if (token[0] == '>')
        {
            fd = FileDescriptor::STDOUT;
        }

        // Detect if it's append (>>)
        size_t arrow_len = 1;
        if (pos < token.size() && token[pos] == '>')
        {
            if (pos + 1 < token.size() && token[pos + 1] == '>')
            {
                append = true;
                arrow_len = 2;
            }
        }

        // If the token itself contains a file (like 1>>out.txt)
        if (pos + arrow_len < token.size())
        {
            std::string file = token.substr(pos + arrow_len);
            result.redirections.push_back({fd, file, append});
            consumed = true;
        }
        // Or if the file is in the next token (like 1>> out.txt)
        else if (token == ">" || token == ">>" || token == "1>" || token == "1>>" || token == "2>" || token == "2>>")
        {
            if (i + 1 < tokens.size())
            {
                fd = (token[0] == '2') ? FileDescriptor::STDERR : FileDescriptor::STDOUT;
                append = (token.find(">>") != std::string::npos);
                result.redirections.push_back({fd, tokens[i + 1], append});
                ++i;
                consumed = true;
            }
        }

        if (!consumed)
        {
            result.tokens.push_back(token);
        }
    }

    return result;
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
