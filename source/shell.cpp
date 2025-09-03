#include "../headers/shell.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm> // for std::find
#include <cstdlib>
#include <unistd.h> 
#include <sys/wait.h>    
#include <limits.h> 
#include <fcntl.h>     // for O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>    // for close(), dup2()
#include <libgen.h>   // for dirname()
#include <cerrno>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>


/*
using name space std cost me alot so I got rid of it.

the tokenize function breaks an input command string into separate tokens while handling single-qouted strings
tokenize("ls  -l   '/home/user dir'");
        -> ["ls", "-l", "/home/user dir"]

handle commands function handles commands as its name suggests 

I used stringstream for parsing, {it's Heavier, slower for concatenation} 
*/

void Shell::ensureParentDir(const std::string& path) {
    size_t pos = path.find_last_of('/'); 
    if(pos == std::string::npos) return; // no directory part

    std::string dir = path.substr(0, pos); 

    struct stat st{}; 
    if(stat(dir.c_str(), &st) != 0) {

        if(mkdir(dir.c_str(), 0755) != 0) {
            perror("mkdir"); 
            exit(1); 
        }
    } else {
        if(!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Error: %s exits and is not a directory\n", dir.c_str()); 
            exit(1); 
        }
    }
}
std::pair<std::vector<std::string>, std::string>
Shell::parseRedirection(const std::vector<std::string>& tokens) {
    std::string redirectFile; 
    int redirectIndex = -1; 

    for(size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ">" || tokens[i] ==  "1>") {
            if (i+1 < tokens.size()) {
                redirectFile = tokens[i+1]; 
                redirectIndex = i; 
            } else {
                std::cerr << "Syntac error near unexpected token `newline`\n"; 
                return {{}, ""}; 
            }
            break; 
        }
    }
    std::vector<std::string> cleanTokens; 
    if (redirectIndex != -1) { 
        cleanTokens.insert(cleanTokens.end(), tokens.begin(), tokens.begin() + redirectIndex); 
    } else {
        cleanTokens = tokens; 
    }
    return {cleanTokens, redirectFile}; 
}


void Shell::setupRedirection(const std::string& redirectFile) { 
    if (!redirectFile.empty()) {
        ensureParentDir(redirectFile); 

        int fd = open(redirectFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); 
        if (fd < 0) {
            perror("open"); 
            exit(1); 
        }
        dup2(fd, STDOUT_FILENO); 
        close(fd); 
    }
}


void Shell::run() {
  std::string input; 
  while(true) {
    std::cout << "$ " << std::flush;
    if (!std::getline(std::cin, input)) break;

    handleCommand(input); 
  }
}

std::vector<std::string> Shell::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string curr;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '\\') {
            if (in_single_quote) {
                curr+= '\\'; 
            } else if (in_double_quote) {
                if(i + 1 < input.size() && 
                    (input[i+1] == '"' || input[i+1] == '\\' ||
                     input[i+1] == '$' || input[i+1] == '`')) {
                    curr += input[i+1];
                    i++; 
            } else {
                curr += '\\'; 
            }
            } else {
                if (i + 1 < input.size()) {
                    curr+= input[i+1]; 
                    i++; 
                }
            }
            continue; 
        } 

        
        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            continue; 
        }

        
        if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            continue; 
        }

        if (!in_single_quote && !in_double_quote && (c == '>')) {
            if (!curr.empty()) {
                tokens.push_back(curr);
                curr.clear();
            }
            tokens.push_back(">"); 
        } else {
            curr.push_back(c); 
        }
    }

    if (!curr.empty()) tokens.push_back(curr);

    return tokens;
}



void Shell::handleCommand(const std::string& input) {
    static const std::vector<std::string> builtins = {"echo", "exit", "type", "pwd", "cd"};
  
    std::vector<std::string> tokens = this->tokenize(input);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];

    if (cmd == "exit") {
        int code = 0;
        if (tokens.size() > 1) {
            code = std::stoi(tokens[1]);
        }
        exit(code);

    } else if (cmd == "echo") {
        // just print everything after "echo"
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (i > 1) std::cout << " ";
            std::cout << tokens[i];
        }
        std::cout << "\n";
        return;

    } else if (cmd == "type") {
        if (tokens.size() < 2) return;
        std::string s = tokens[1];
        if (std::find(builtins.begin(), builtins.end(), s) != builtins.end()) {
            std::cout << s << " is a shell builtin\n";
            return;
        }
        char* pathEnv = getenv("PATH");
        bool found = false;
        if (pathEnv) {
            std::istringstream paths(pathEnv);
            std::string dir;
            while (std::getline(paths, dir, ':')) {
                std::string fullPath = dir + "/" + s;
                if (access(fullPath.c_str(), X_OK) == 0) {
                    std::cout << s << " is " << fullPath << "\n";
                    found = true;
                    break;
                }
            }
        }
        if (!found) std::cout << s << ": not found\n";
        return;

    } else if (cmd == "pwd") {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::cout << cwd << "\n";
        } else {
            perror("getcwd");
        }
        return;

    } else if (cmd == "cd") {
        if (tokens.size() < 2) return;
        std::string path = tokens[1];
        if (path == "~") {
            char* home = getenv("HOME");
            if (!home || chdir(home) != 0) {
                std::cout << "$\n";
            }
        } else {
            if (chdir(path.c_str()) != 0) {
                std::cout << "$\n"
            }
        }
        return;

    } else {
        // external command
        /*
            execvp() requires arguments as an array of C-style strings (char*[]), not C++ std::string
            
            fork() duplicates the current process: 
                            the parent process continues running 
                            the child process is a clone that can be used to run the command
            if pid == 0 then we r in the child process
            if pid > 0 then we r in the parent process
            otherwise fork failed

            parent shell waits for the child process to finsih using waitpid
            perror -> error handling 

            if execvp succeeds, the current child process becomes the command other wise it prints not found message
            -> the parent shell waits for the child process to finish using waitpid
            -> this prevents creating ZOMBIE processes AND ensures commands finish before showing the nest shell prompt.
        */
    auto [cleanTokens, redirectFile] = parseRedirection(tokens);
    if (cleanTokens.empty()) return;

    std::vector<char*> argv;
    for (auto &t : cleanTokens) {
        argv.push_back(const_cast<char*>(t.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        setupRedirection(redirectFile); 
        execvp(argv[0], argv.data());
        perror("execvp failed");
        exit(1);
    } else {
        waitpid(pid, nullptr, 0);
        }

    }
}