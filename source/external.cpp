#include "../headers/shell.hpp"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <string>

void Shell::executeExternal(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return;

    auto [cleanTokens, redirs] = parseRedirection(tokens);
    if (cleanTokens.empty()) return;

    std::vector<char*> argv;
    std::vector<std::string> argv_storage = cleanTokens;
    for (auto &t : argv_storage) {
        argv.push_back(const_cast<char*>(t.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        setupRedirection(redirs.stdoutRedir, redirs.stderrRedir);
        execvp(argv[0], argv.data());
        std::cerr << argv[0] << ": command not found\n";
        exit(127);
    } else if (pid > 0) {
        waitpid(pid, nullptr, 0);
    } else {
        perror("fork");
    }
}
