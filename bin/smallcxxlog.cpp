/// @file bin/smallcxxlog.cpp
/// @brief Print a log message from the command line
/// @author Christopher White <cxwembedded@gmail.com>
/// @copyright Copyright (c) 2021 Christopher White

#include <cstdlib>
#include <stdio.h>

#include <smallcxx/logging.hpp>

using std::atoi;

int
main(int argc, char **argv)
{
    if(argc != 6) {
        fprintf(stderr, "Usage: %s LEV FILE LINE FUNCTION 'MESSAGE'\n", argv[0]);
        return 2;
    }

    setLogLevel(
        LOG_MAX);   // so the message will always print (TODO make this an option?)

    // Parse cmdline
    LogLevel level = (LogLevel)atoi(argv[1]);     // TODO parse level names
    if(level == LOG_SILENT) {   // if atoi() couldn't parse ...
        level = LOG_FIXME;      // ... print it as fixme.
    }

    const char *file = argv[2];
    const int line = atoi(argv[3]);
    const char *function = argv[4];
    const char *msg = argv[5];

    logMessage(SMALLCXX_DEFAULT_LOG_DOMAIN, level,
               file, line, function, "%s", msg);
    return 0;
}
