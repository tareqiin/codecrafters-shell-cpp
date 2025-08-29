#include "../headers/shell.hpp"
#include <iostream>
#include <sstream>


void Shell::run() {
  std::string input; 
  while(true) {
    std::cout << "$ "; 
    if (!std::getline(std::cin, input)) break;

    handleCommand(input); 
  }
}

void Shell::handleCommand(const std::string& input) {
  std::istringstream iss(input); 
  std::string cmd; 
  iss >> cmd; 

  if (cmd == "exit") {
    int code = 0; 
    if (iss >> code) {
      exit(code); 
    }
    exit(0); 
  }

  std::cout << input << ": command not found\n"; 
}