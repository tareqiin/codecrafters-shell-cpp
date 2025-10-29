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
    pr.tokens = tokens;

    for (size_t i = 0; i < pr.tokens.size();) {
        const std::string &tok = pr.tokens[i];

        bool handled = false;
        // simple stdout redirection (truncate)
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
        // append stdout
        } else if (tok == ">>" || tok == "1>>") {
            if (i + 1 < pr.tokens.size()) {
                pr.stdoutFile = pr.tokens[i + 1];
                pr.stdoutAppend = true;
                pr.tokens.erase(pr.tokens.begin() + i, pr.tokens.begin() + i + 2);
                handled = true;
            } else {
                std::cerr << "Syntax error near unexpected token `newline`\n";
                return ParseResult{};
            }
        // stderr redirections (2> / 2>>)
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
        // numeric-fd forms like "3>file" or "3> file" or "4>>out"
        } else if (!tok.empty() && std::isdigit(static_cast<unsigned char>(tok[0]))) {
            // parse leading digits then > or >>
            size_t j = 0;
            while (j < tok.size() && std::isdigit(static_cast<unsigned char>(tok[j]))) ++j;
            bool hasGreater = (j < tok.size() && tok[j] == '>');
            bool hasDoubleGreater = (j + 1 < tok.size() && tok[j] == '>' && tok[j+1] == '>');
            if (!hasGreater && !hasDoubleGreater) {
                // not a fd redirection token -> fall through (not handled)
            } else {
                std::string fdStr = tok.substr(0, j);
                int fd = 0;
                try {
                    fd = std::stoi(fdStr);
                } catch (...) {
                    std::cerr << "Syntax error: invalid file descriptor\n";
                    return ParseResult{};
                }

                bool append = hasDoubleGreater;
                size_t sepLen = append ? 2 : 1;
                std::string filename;
                if (j + sepLen < tok.size()) {
                    // filename attached in same token: e.g. "3>file" or "4>>out"
                    filename = tok.substr(j + sepLen);
                    // erase only this token
                    pr.tokens.erase(pr.tokens.begin() + i);
                } else {
                    // filename must be next token
                    if (i + 1 >= pr.tokens.size()) {
                        std::cerr << "Syntax error near unexpected token `newline'\n";
                        return ParseResult{};
                    }
                    filename = pr.tokens[i + 1];
                    // erase this token and the filename token
                    pr.tokens.erase(pr.tokens.begin() + i, pr.tokens.begin() + i + 2);
                }

                // assign to stdout/stderr OR ignore/extend for other fds
                if (fd == 1) {
                    pr.stdoutFile = filename;
                    pr.stdoutAppend = append;
                } else if (fd == 2) {
                    pr.stderrFile = filename;
                    pr.stderrAppend = append;
                } else {
                    // Currently ParseResult doesn't store arbitrary fd mappings.
                    // You can extend ParseResult with a map<int, pair<string,bool>> if you want to support them.
                    // For now we silently ignore other fds.
                }
                handled = true;
            }
        }

        if (!handled) {
            ++i;
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
