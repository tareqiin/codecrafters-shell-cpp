#include "../headers/shell.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm> // for std::find
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>     // for O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>    // for close(), dup2()
#include <libgen.h>   // for dirname()
#include <cerrno>
#include <string>
#include <sys/types.h>
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


void Shell::run() {
    std::string input;
    while (true) {
        std::cout << "$ " << std::flush;
        if (!std::getline(std::cin, input)) break;

        handleCommand(input);
    }
}

std::vector<std::string> Shell::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string curr;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '\\') {
            if (in_single_quote) {
                curr += '\\';
            } else if (in_double_quote) {
                if (i + 1 < input.size() &&
                    (input[i+1] == '"' || input[i+1] == '\\' ||
                     input[i+1] == '$' || input[i+1] == '`')) {
                    curr += input[i+1];
                    i++;
                } else {
                    curr += '\\';
                }
            } else {
                if (i + 1 < input.size()) {
                    curr += input[i+1];
                    i++;
                }
            }
            continue;
        }

        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            continue;
        }

        if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            continue;
        }
        if (!in_single_quote && !in_double_quote && c == '>') {
            if (!curr.empty()) {
                tokens.push_back(curr);
                curr.clear();
            }
            tokens.push_back(">");
            continue;
        }

        if (!in_single_quote && !in_double_quote && std::isspace(static_cast<unsigned char>(c))) {
            if (!curr.empty()) {
                tokens.push_back(curr);
                curr.clear();
            }
            continue;
        }
        curr.push_back(c);
    }

    if (!curr.empty()) tokens.push_back(curr);
    return tokens;
}


void Shell::handleCommand(const std::string& input) {
    static const std::vector<std::string> builtins = {"echo", "exit", "type", "pwd", "cd"};

    std::vector<std::string> tokens = this->tokenize(input);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];

    if (cmd == "exit") {
        int code = 0;
        if (tokens.size() > 1) {
            code = std::stoi(tokens[1]);
        }
        exit(code);

    } else if (cmd == "echo") {
        auto [cleanTokens, redirs] = parseRedirection(tokens);
        std::string stdoutFile = redirs.first;
        std::string stderrFile = redirs.second;


        int savedStdout = redirectStdoutToFile(stdoutFile);
        int savedStderr = redirectStderrToFile(stderrFile);

        for (size_t i = 1; i < cleanTokens.size(); ++i) {
            if (i > 1) std::cout << " ";
            std::cout << cleanTokens[i];
        }
        std::cout << "\n";
        std::cout.flush();

        if (savedStdout >= 0) { dup2(savedStdout, STDOUT_FILENO); close(savedStdout); }
        if (savedStderr >= 0) { dup2(savedStderr, STDERR_FILENO); close(savedStderr); }
        return;

    } else if (cmd == "type") {
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
        return;

    } else if (cmd == "pwd") {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::cout << cwd << "\n";
        } else {
            perror("getcwd");
        }
        return;

    } else if (cmd == "cd") {
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
        return;

    } else {
        // external command
    auto [cleanTokens, redirs] = parseRedirection(tokens);
    std::string stdoutFile = redirs.first;
    std::string stderrFile = redirs.second;

    if (cleanTokens.empty()) return;

    std::vector<char*> argv;
    std::vector<std::string> argv_storage = cleanTokens;
    for (auto &t : argv_storage) {
        argv.push_back(const_cast<char*>(t.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        setupRedirection(stdoutFile, stderrFile);
        execvp(argv[0], argv.data());
        std::cerr << argv[0] << ": command not found\n";
        std::cout.flush();
        exit(127);
        } else if (pid > 0) {
            waitpid(pid, nullptr, 0);
            } else {
                perror("fork");
            }
}

}
