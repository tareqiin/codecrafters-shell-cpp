#include <iostream>
#include <string>

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  
  while(true) {
    std::cout << "$ ";
    if(!std::getline(std::cin, input))return 0; 
    if(input == "exit") return 0
    std::cout << input << ": command not found\n"; 
    
  }
}
