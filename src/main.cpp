#include <iostream>
#include <string>

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  
  while(true) {
    if(!std::getline(std::cin, input))break; 
    std::cout << "$ ";
    std::getline(std::cin, input); 
    std::cout << input << ": command not found" << std::endl; 
  }
}
