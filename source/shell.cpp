#include "../headers/shell.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm> // for std::find

void Shell::run() {
  std::string input; 
  while(true) {
    std::cout << "$ "; 
    if (!std::getline(std::cin, input)) break;

    handleCommand(input); 
  }
}

void Shell::handleCommand(const std::string& input) {
  std::vector<std::string> builtins = {"echo", "exit", "type"};
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
  } else if (cmd == "type") { 
     std::string s; iss>>s;
      if(std::find(builtins.begin(), builtins.end(), s) != builtins.end())
      std::cout << s << " is a shell builtin\n";
      else std::cout << "invalid_command: not found\n"; 


      return; 
    }
  std::cout << input << ": command not found\n"; 
}