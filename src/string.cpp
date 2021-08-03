/// @file src/string.cpp
/// @brief String-related definitions for smallcxx - implementation
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include <string.h>

#define SMALLCXX_USE_CHOMP
#include "smallcxx/string.hpp"

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
