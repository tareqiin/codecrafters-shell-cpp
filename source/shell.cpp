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
    std::cout << "$ " << std::flush;
    if (!std::getline(std::cin, input)) break;

    handleCommand(input); 
  }
}

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string curr;
    bool in_single_quote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '\'') {
            in_single_quote = !in_single_quote;
            continue; 
        }

        if (!in_single_quote && std::isspace(static_cast<unsigned char>(c))) {
            if (!curr.empty()) {
                tokens.push_back(curr);
                curr.clear();
            }
        } else {
            curr += c;
        }
    }

    if (!curr.empty()) {
        tokens.push_back(curr);
    }

    return tokens;
}


void Shell::handleCommand(const std::string& input) {
 static const std::vector<std::string> builtins = {"echo", "exit", "type", "pwd", "cd"};
  
  std::vector<std::string> tokens = tokenize(input); 
  std::string cmd = tokens[0]; 

  if (cmd.empty()) return;

  if (cmd == "exit") {
    int code = 0; 
    if (iss >> code) {
      exit(code); 
    }
    exit(0); 
  } else if (cmd == "echo") {
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (i > 1) std::cout << " ";
            std::cout << tokens[i];
        }
        std::cout << "\n";
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
        if (path.empty()) return;

        if (path == "~") {
          char* home = getenv("HOME"); 
          if(home && chdir(home) != 0) {
            std::cout << "cd: " << path << ": No such file or directory\n" << std::flush; 
          }
          return; 
        }

        if (chdir(path.c_str()) != 0) {
            std::cout << "cd: " << path << ": No such file or directory\n" << std::flush; 
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