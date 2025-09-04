#include "../headers/shell.hpp"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <string>

void Shell::executeExternal(const std::vector<std::string>& tokens) {
    auto [cleanTokens, redirectFile] = parseRedirection(tokens);
    if (cleanTokens.empty()) return;

    // Build argv
    std::vector<char*> argv;
    std::vector<std::string> argv_storage = cleanTokens;
    for (auto &t : argv_storage) {
        argv.push_back(const_cast<char*>(t.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        // Child process: setup redirection then exec
        setupRedirection(redirs.first, redirs.second); 
        execvp(argv[0], argv.data());

        // If execvp fails
        std::cerr << argv[0] << ": command not found\n";
        std::cout.flush();
        exit(127);
    } else if (pid > 0) {
        // Parent waits
        waitpid(pid, nullptr, 0);
    } else {
        perror("fork");
    }
}
