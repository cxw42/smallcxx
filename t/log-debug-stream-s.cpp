/// @file log-debug-stream-s.cpp
/// @brief Log a message at LOG_DEBUG level using LOG_S
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021--2022 Christopher White

#include "smallcxx/logging.hpp"

using namespace std;

int
main()
{
    setVerbosityFromEnvironment();
    LOG_S(DEBUG) << "avocado";
    return 0;
}
