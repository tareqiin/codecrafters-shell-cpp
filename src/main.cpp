#include <iostream>
#include <string>

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  
std::string command;
while (true) {
    std::cout << "$ ";  // print prompt
    if (!std::getline(std::cin, command)) break;

    if (command == "exit") break;
    // ... check builtins, etc. ...

    // If not found:
    printCommandNotFound(command);
}

}
