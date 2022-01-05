/// @file t/silent-s.cpp
/// @brief Call log functions after setting logs to silent
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2022 Christopher White

#include "smallcxx/logging.hpp"

using namespace std;

int
main()
{
    setVerbosityFromEnvironment("LOG_LEVELS");
    silenceLog();

    LOG_F(ERROR, "error");
    LOG_F(WARNING, "warning");
    LOG_F(FIXME, "fixme");
    LOG_F(INFO, "info");
    LOG_F(DEBUG, "debug");
    LOG_F(LOG, "log");
    LOG_F(TRACE, "trace");
    LOG_F(PEEK, "peek");
    LOG_F(SNOOP, "snoop");

    // TODO add non-default domains

    return 0;
}
