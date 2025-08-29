#include <iostream>
#include <string>

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

    std::string input;
    while (true) {
        std::cout << "$ ";
        if (!std::getline(std::cin, input)) return 0;

        std::istringstream iss(input);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit") {
            int code = 0; // default exit code
            if (iss >> code) {
                return code;  // exit with provided code
            }
            return 0; // no code → exit 0
        }

        std::cout << input << ": command not found\n";
    }
}
