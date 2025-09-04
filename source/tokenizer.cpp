#include "../headers/shell.hpp"
#include <vector> 
#include <string> 
#include <cctype>
#include <algorithm>

std::vector<std::string> Shell::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string curr;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '\\') {
            if (in_single_quote) {
                curr += '\\';
            } else if (in_double_quote) {
                if (i + 1 < input.size() &&
                    (input[i+1] == '"' || input[i+1] == '\\' ||
                     input[i+1] == '$' || input[i+1] == '`')) {
                    curr += input[i+1];
                    i++;
                } else {
                    curr += '\\';
                }
            } else {
                if (i + 1 < input.size()) {
                    curr += input[i+1];
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


        if (!in_single_quote && !in_double_quote && c == '>') {
            bool append = (i + 1 < input.size() && input[i+1] == '>'); 
            if(!curr.empty() && std::all_of(curr.begin(), curr.end(), [](unsigned char ch){ return std::isdigit(ch); })) {
                if(append) {
                    tokens.push_back(curr + ">>"); 
                    curr.clear(); 
                    i++; 
                } else {
                    tokens.push_back(curr + ">"); 
                    curr.clear(); 
                }
            } else {
                if(!curr.empty()) {
                    tokens.push_back(curr); 
                    curr.clear(); 
                } else {
                    if(append) {
                        tokens.push_back(">>"); 
                        i++; 
                    } else {
                        tokens.push_back(">"); 
                    }
                }
                continue; 
            }
            if (!curr.empty()) {
                tokens.push_back(curr);
                curr.clear();
            }
            if (!tokens.empty() && (tokens.back() == "1" || tokens.back() == "2")) {
                std::string fd = tokens.back(); 
                tokens.pop_back(); 
                tokens.push_back(fd + ">"); 
            } else {
                tokens.push_back(">"); 
            }
            continue;
        }

        if (!in_single_quote && !in_double_quote && std::isspace(static_cast<unsigned char>(c))) {
            if (!curr.empty()) {
                tokens.push_back(curr);
                curr.clear();
            }
            continue;
        }
        curr.push_back(c);
    }

    if (!curr.empty()) tokens.push_back(curr);
    return tokens;
}