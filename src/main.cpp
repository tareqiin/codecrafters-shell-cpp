#include <iostream>
#include <string>

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
    while(true) {
      using namespace std; 
      cout << "$ "; 
      if(!getline(cin, input)) return 0; 

      istringstream iss(input); 
      string cmd; 
      iss >> cmd; 

      if (cmd == "exit") {
        int code = 0; 
        if (iss >> code) {
          return code; 
        }
        return 0; 
      }

      cout << input << ": command not found\n"; 
  }
}
