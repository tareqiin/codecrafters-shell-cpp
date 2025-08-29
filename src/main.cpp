#include <iostream>
#include <string>
#include "../headers/shell.hpp"

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  Shell shell; 
  shell.run(); 
  return 0; 
}
