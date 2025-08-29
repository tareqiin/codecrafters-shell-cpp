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
  } else if (cmd == "echo") {
    std::string s;
    std::getline(iss, s); 
    if(!s.empty() && s[0] == ' ') s.erase(0,1); 
    std::cout << s << "\n"; 
    return; 
  }

  std::cout << input << ": command not found\n"; 
}