/// @file t/varying-log-s.cpp
/// @brief Log, but change the default halfway through.
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2022 Christopher White

#define SMALLCXX_LOG_DOMAIN "lev"
#include "smallcxx/logging.hpp"

using namespace std;

int
main()
{

    LOG_F(ERROR, "error1");
    LOG_F(WARNING, "warning1");
    LOG_F(FIXME, "fixme1");
    LOG_F(INFO, "info1");
    LOG_F(DEBUG, "debug1");
    LOG_F(LOG, "log1");
    LOG_F(TRACE, "trace1");
    LOG_F(PEEK, "peek1");
    LOG_F(SNOOP, "snoop1");

    // Change the default, if $LOG_LEVELS includes a `*:N` term
    setVerbosityFromEnvironment("LOG_LEVELS");

    LOG_F(ERROR, "error2");
    LOG_F(WARNING, "warning2");
    LOG_F(FIXME, "fixme2");
    LOG_F(INFO, "info2");
    LOG_F(DEBUG, "debug2");
    LOG_F(LOG, "log2");
    LOG_F(TRACE, "trace2");
    LOG_F(PEEK, "peek2");
    LOG_F(SNOOP, "snoop2");

    return 0;
}
