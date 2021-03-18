/// @file log-debug-message-s.cpp
/// @brief Helper used by t/v-env-var.sh
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include "smallcxx/logging.hpp"

using namespace std;

int
main()
{
    setVerbosityFromEnvironment();
    LOG_F(DEBUG, "avocado");
    return 0;
}
