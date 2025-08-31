#include "../headers/shell.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm> // for std::find
#include <cstdlib>
#include <unistd.h> 
#include <sys/wait.h>    
#include <limits.h> 

void Shell::run() {
  std::string input; 
  while(true) {
    std::cout << "$ "; 
    if (!std::getline(std::cin, input)) break;

    handleCommand(input); 
  }
}

void Shell::handleCommand(const std::string& input) {
 static const std::vector<std::string> builtins = {"echo", "exit", "type", "pwd", "cd"};
  std::istringstream iss(input); 
  std::string cmd; 
  iss >> cmd; 
  if (cmd.empty()) return;

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
      if(std::find(builtins.begin(), builtins.end(), s) != builtins.end()) {
        std::cout << s << " is a shell builtin\n";
        return; 
      }

      char* pathEnv = getenv("PATH"); 
      bool found = false; 
      if(pathEnv) {
        std::istringstream paths(pathEnv); 
        std::string dir; 
        while(std::getline(paths, dir, ':')) {
          std::string fullPath = dir + "/" + s; 
          if(access(fullPath.c_str(), X_OK) == 0) {
            std::cout << s << " is " << fullPath << "\n"; 
            found = true; 
            break; 
          }
        }
      }
      if(!found) std::cout << s << ": not found\n"; 
      return; 
    } else if(cmd == "pwd") {
      char cwd[PATH_MAX]; // this is a buffer
      // getcwd stands for "get current working directory"
      if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << cwd << "\n"; 
      } else {
        perror("getcwd");  
      }
      return; 
    } else if (cmd == "cd") {
      std::string path; 
      iss >> path; 

      if(path.empty()) return; 
      

      if(path[0] == '/') {
        if (chdir(path.c_str()) != 0) std::cout << "cd: " << path << ": No such file or directory\n";
        else std::cout << "cd: " << path << ": No such file or directory\n";
      }
      return; 
    }
    else {
      std::vector<std::string> tokens; 
      tokens.push_back(cmd); 
      std::string arg; 
      while(iss >> arg) tokens.push_back(arg); 

      std::vector<char*> argv; 
      for(auto &t : tokens) argv.push_back(const_cast<char*>(t.c_str())); 
      argv.push_back(nullptr); 

      pid_t pid = fork();  // pid = process id, and fork is system call in Unix/Linux that creates a new process. 
      if(pid == 0) {
        // child
        execvp(argv[0], argv.data()); 
        std::cout << argv[0] << ": command not found\n";  
        exit(1); 
      } else if (pid > 0) {
        // parent 
        int status; 
        waitpid(pid, &status, 0); // waitpid is a system call in Unix/Linux used by a parent process to wait for a specific child process to finish.
      } else {
        perror("fork"); 
      }
      return; 
    }
    
}