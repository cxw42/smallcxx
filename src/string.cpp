/// @file src/string.cpp
/// @brief String-related definitions for smallcxx - implementation
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include <algorithm>
#include <cctype>
#include <string.h>

#define SMALLCXX_USE_CHOMP
#include "smallcxx/string.hpp"

using namespace std;

void
chomp(char *str)
{
    if(str[0] == '\0') {
        return;
    }
    if(str[strlen(str) - 1] == '\n') {
        str[strlen(str) - 1] = '\0';
    }
}

namespace smallcxx
{

std::string
trim(const std::string& s)
{
    size_t first, last;
    for(first = 0; first < s.length(); ++first) {
        if(!isspace(s[first])) {
            break;
        }
    }

    for(last = s.length() - 1; last >= 0; --last) {
        if(!isspace(s[last])) {
            break;
        }
    }

    return s.substr(first, last - first + 1);
}

} // namespace smallcxx
