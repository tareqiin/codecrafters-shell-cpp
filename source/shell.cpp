#include "../headers/shell.hpp"


void Shell::run() {
    std::string input;
    while (true) {
        std::cout << "$ " << std::flush;
        if (!std::getline(std::cin, input)) break;

        handleCommand(input);
    }
}




void Shell::handleCommand(const std::string& input) {
    std::vector<std::string> tokens = this->tokenize(input);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];

    if (cmd == "exit") {
        builtinExit(tokens);
    } else if (cmd == "echo") {
        builtinEcho(tokens);
    } else if (cmd == "type") {
        builtinType(tokens);
    } else if (cmd == "pwd") {
        builtinPwd(tokens);
    } else if (cmd == "cd") {
        builtinCd(tokens);
    } else {
        executeExternal(tokens); 
    }
}
