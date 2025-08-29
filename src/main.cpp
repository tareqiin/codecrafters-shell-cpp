#include <iostream>
#include <string>

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  
  while(true) {
    std::cout << "$ ";
    if(!std::getline(std::cin, input))break; 
    std::cout << input << ": command not found\n"; 
    std::cout << "$ " << "exit 0\n"; 
  }
}
